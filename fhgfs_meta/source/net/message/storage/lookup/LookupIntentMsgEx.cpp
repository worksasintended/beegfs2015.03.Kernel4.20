#include <program/Program.h>
#include <common/net/message/storage/lookup/LookupIntentRespMsg.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/toolkit/SessionTk.h>
#include <net/msghelpers/MsgHelperMkFile.h>
#include <net/msghelpers/MsgHelperOpen.h>
#include <net/msghelpers/MsgHelperStat.h>
#include <session/SessionStore.h>
#include <storage/DentryStoreData.h>
#include "LookupIntentMsgEx.h"


bool LookupIntentMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{

   #ifdef BEEGFS_DEBUG
      const char* msgInContext = "LookupIntentMsg incoming";
      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(msgInContext, 4, std::string("Received a LookupIntentMsg from: ") + peer);
   #endif

   App* app = Program::getApp();

   std::string parentEntryID = getParentInfo()->getEntryID();

   FhgfsOpsErr lookupRes = FhgfsOpsErr_INTERNAL;
   FhgfsOpsErr createRes = FhgfsOpsErr_INTERNAL;

   std::string entryName = getEntryName();

   const char* logContext = "LookupIntentMsg";

   LOG_DEBUG(logContext, 5, "parentID: '" + parentEntryID + "' " +
      "entryName: '" + entryName + "'");


   // (note: following objects must be at the top-level stack frame for response serialization)
   std::string fileHandleID;
   Raid0Pattern dummyPattern(1, UInt16Vector() );
   EntryInfo diskEntryInfo;

   int createFlag = getIntentFlags() & LOOKUPINTENTMSG_FLAG_CREATE;
   FileInodeStoreData inodeData;
   bool inodeDataOutdated = false; // true if the file/inode is currently open (referenced)
   LookupIntentRespMsg respMsg(FhgfsOpsErr_INTERNAL);

   PathInfo pathInfo; /* Added to NetMessage as ref-pointer, so object needs to exist until
                       * the NetMessage got serialized! */

   // sanity checks
   if (unlikely (parentEntryID.empty() || entryName.empty() ) )
   {
      LogContext(logContext).log(3, "Sanity check failed: parentEntryID: '"
         + parentEntryID + "'" + "entryName: '" + entryName + "'");

      // something entirely wrong, fail immediately, error already set above
      goto send_response;
   }


   /* Note: Actually we should first do a lookup. However, a successful create also implies a
             failed Lookup, so we can take a shortcut. */

   // lookup-create
   if (createFlag)
   {
      LOG_DEBUG(logContext, Log_SPAM, "Lookup: create");

      // checks in create() if lookup already found the entry
      createRes = create(this->getParentInfo(), entryName, &diskEntryInfo,
         &inodeData);

      FhgfsOpsErr sendCreateRes;
      if (createRes == FhgfsOpsErr_SUCCESS)
      {
         sendCreateRes = FhgfsOpsErr_SUCCESS;

         // Successful Create, which implies Lookup-before-create would have been failed.
         respMsg.setLookupResult(FhgfsOpsErr_PATHNOTEXISTS);

         respMsg.setEntryInfo(&diskEntryInfo);
      }
      else
      {
         if (createRes == FhgfsOpsErr_EXISTS)
         {
            // NOTE: we need to do a Lookup to get required lookup data

            if (getIntentFlags() & LOOKUPINTENTMSG_FLAG_CREATEEXCLUSIVE)
               sendCreateRes = FhgfsOpsErr_EXISTS;
            else
               sendCreateRes = FhgfsOpsErr_SUCCESS;

         }
         else
            sendCreateRes = FhgfsOpsErr_INTERNAL;
      }

      respMsg.addResponseCreate(sendCreateRes);

      // note: don't quit here on error because caller might still have requested stat info
   }

   // lookup
   if ((!createFlag) || (createRes == FhgfsOpsErr_EXISTS) )
   {
      LOG_DEBUG(logContext, Log_SPAM, "Lookup: lookup");

      lookupRes = lookup(parentEntryID, entryName, &diskEntryInfo, &inodeData, inodeDataOutdated);

      respMsg.setLookupResult(lookupRes);

      if (lookupRes == FhgfsOpsErr_SUCCESS)
         respMsg.setEntryInfo(&diskEntryInfo);

      if(unlikely( (lookupRes != FhgfsOpsErr_SUCCESS) && createFlag) )
      {
         // so createFlag is set, so createRes is either Success or Exists, but now lookup fails
         // create/unlink race?

         // we need to set something here, as sendCreateRes = FhgfsOpsErr_SUCCESS
         respMsg.setEntryInfo(&diskEntryInfo);

         StatData statData;
         statData.setAllFake(); // set arbitrary stat values (receiver won't use the values)

         respMsg.addResponseStat(lookupRes, &statData);

         goto send_response;
      }
   }

   // lookup-revalidate
   if(getIntentFlags() & LOOKUPINTENTMSG_FLAG_REVALIDATE)
   {
      LOG_DEBUG(logContext, Log_SPAM, "Lookup: revalidate");

      FhgfsOpsErr revalidateRes = revalidate(&diskEntryInfo);

      respMsg.addResponseRevalidate(revalidateRes);

      if(revalidateRes != FhgfsOpsErr_SUCCESS)
         goto send_response;
   }


   /* lookup-stat
      note: we do stat before open to avoid the dyn attribs refresh if the file is not opened by
      someone else currently. */
   if ( (getIntentFlags() & LOOKUPINTENTMSG_FLAG_STAT) &&
        (lookupRes == FhgfsOpsErr_SUCCESS || createRes == FhgfsOpsErr_SUCCESS) )
   {
      LOG_DEBUG(logContext, Log_SPAM, "Lookup: stat");

      // check if lookup and create failed (we don't have an entryID to stat then)
      if(diskEntryInfo.getEntryID().empty() )
         goto send_response;

      if ( (diskEntryInfo.getFlags() & ENTRYINFO_FEATURE_INLINED) && !inodeDataOutdated)
      {  // stat-data from the dentry
         StatData* dentryStatData = inodeData.getInodeStatData();

         respMsg.addResponseStat(FhgfsOpsErr_SUCCESS, dentryStatData);
      }
      else
      {  // read stat data separately
         StatData statData;
         FhgfsOpsErr statRes = stat(&diskEntryInfo, true, statData);

         respMsg.addResponseStat(statRes, &statData);

         if(statRes != FhgfsOpsErr_SUCCESS)
            goto send_response;
      }

   }

   // lookup-open
   if(getIntentFlags() & LOOKUPINTENTMSG_FLAG_OPEN)
   {
      LOG_DEBUG(logContext, Log_SPAM, "Lookup: open");

      // don't open if create failed
      if ((createRes != FhgfsOpsErr_SUCCESS) && (getIntentFlags() & LOOKUPINTENTMSG_FLAG_CREATE) )
         goto send_response;

      if (!DirEntryType_ISREGULARFILE(diskEntryInfo.getEntryType() ) )
         goto send_response; // not a regular file, we don't open that

      // check if lookup and/or create failed (we don't have an entryID to open then)
      if(diskEntryInfo.getEntryID().empty() )
         goto send_response;

      StripePattern* pattern = NULL;

      FhgfsOpsErr openRes = open(&diskEntryInfo, &fileHandleID, &pattern, &pathInfo);

      if(openRes != FhgfsOpsErr_SUCCESS)
      { // open failed => use dummy pattern for response
         respMsg.addResponseOpen(openRes, fileHandleID, &dummyPattern, &pathInfo);

         goto send_response;
      }

      respMsg.addResponseOpen(openRes, fileHandleID, pattern, &pathInfo);
   }


send_response:

   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );


   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), this->getOpCounterType(),
      getMsgHeaderUserID() );

   return true;
}

FhgfsOpsErr LookupIntentMsgEx::lookup(std::string& parentEntryID, std::string& entryName,
   EntryInfo* outEntryInfo, FileInodeStoreData* outInodeStoreData, bool& outInodeDataOutdated)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   DirInode* parentDir = metaStore->referenceDir(parentEntryID, false);
   if(!parentDir)
   { // out of memory
      return FhgfsOpsErr_INTERNAL;
   }

   FhgfsOpsErr lookupRes = metaStore->getEntryData(parentDir, entryName, outEntryInfo,
      outInodeStoreData);

   if (lookupRes == FhgfsOpsErr_DYNAMICATTRIBSOUTDATED)
   {
      lookupRes = FhgfsOpsErr_SUCCESS;
      outInodeDataOutdated = true;
   }

   metaStore->releaseDir(parentEntryID);

   return lookupRes;
}

/**
 * compare entryInfo on disk with EntryInfo send by the client
 * @return FhgfsOpsErr_SUCCESS if revalidation successful, FhgfsOpsErr_PATHNOTEXISTS otherwise.
 */
FhgfsOpsErr LookupIntentMsgEx::revalidate(EntryInfo* diskEntryInfo)
{
   EntryInfo* clientEntryInfo = this->getEntryInfo();

   if ( (diskEntryInfo->getEntryID()     == clientEntryInfo->getEntryID() ) &&
        (diskEntryInfo->getOwnerNodeID() == clientEntryInfo->getOwnerNodeID() ) )
      return FhgfsOpsErr_SUCCESS;


   return FhgfsOpsErr_PATHNOTEXISTS;
}

FhgfsOpsErr LookupIntentMsgEx::create(EntryInfo* parentInfo,
   std::string& entryName, EntryInfo *outEntryInfo, FileInodeStoreData* outInodeData)
{
   const int umask = isMsgHeaderFeatureFlagSet(LOOKUPINTENTMSG_FLAG_UMASK)
      ? getUmask()
      : 0000;

   MkFileDetails mkDetails(entryName, getUserID(), getGroupID(), getMode(), umask );

   UInt16List preferredTargets;
   parsePreferredTargets(&preferredTargets);

   return MsgHelperMkFile::mkFile(parentInfo, &mkDetails, &preferredTargets, 0, 0,
      outEntryInfo, outInodeData);
}

FhgfsOpsErr LookupIntentMsgEx::stat(EntryInfo* entryInfo, bool loadFromDisk, StatData& outStatData)
{
   Node* localNode = Program::getApp()->getLocalNode();

   FhgfsOpsErr statRes = FhgfsOpsErr_NOTOWNER;

   // check if we can stat on this machine or if entry is owned by another server
   if(entryInfo->getOwnerNodeID() == localNode->getNumID() )
      statRes = MsgHelperStat::stat(entryInfo, loadFromDisk, getMsgHeaderUserID(), outStatData);

   return statRes;
}

/**
 * @param outPattern only set if success is returned; points to referenced open file, so it does
 * not need to be free'd/deleted.
 */
FhgfsOpsErr LookupIntentMsgEx::open(EntryInfo* entryInfo, std::string* outFileHandleID,
   StripePattern** outPattern, PathInfo* outPathInfo)
{
   App* app = Program::getApp();
   SessionStore* sessions = app->getSessions();

   FileInode* inode;

   bool useQuota = isMsgHeaderFeatureFlagSet(LOOKUPINTENTMSG_FLAG_USE_QUOTA);

   FhgfsOpsErr openRes = MsgHelperOpen::openFile(
      entryInfo, getAccessFlags(), useQuota, getMsgHeaderUserID(), &inode);

   if(openRes != FhgfsOpsErr_SUCCESS)
      return openRes; // error occurred

   // open successful => insert session

   SessionFile* sessionFile = new SessionFile(inode, getAccessFlags(), entryInfo);

   Session* session = sessions->referenceSession(getSessionID(), true);

   *outPattern = inode->getStripePattern();
   inode->getPathInfo(outPathInfo);

   unsigned ownerFD = session->getFiles()->addSession(sessionFile);

   sessions->releaseSession(session);

   *outFileHandleID = SessionTk::generateFileHandleID(ownerFD, entryInfo->getEntryID() );

   return openRes;
}

/**
 * Decide which type of client stats op counter we increase for this msg (based on given msg flags).
 */
MetaOpCounterTypes LookupIntentMsgEx::getOpCounterType()
{
   /* note: as introducting a speparate opCounter type for each flag would have been too much,
      we assign prioritiess here as follows: create > open > revalidate > stat > simple_no_flags */

   /* NOTE: Those if's are rather slow, maybe we should create a table that has the values?
    *       Problem is that the table has to be filled for flag combinations, which is also ugly
    */

   if(this->getIntentFlags() & LOOKUPINTENTMSG_FLAG_CREATE)
      return MetaOpCounter_LOOKUPINTENT_CREATE;

   if(this->getIntentFlags() & LOOKUPINTENTMSG_FLAG_OPEN)
      return MetaOpCounter_LOOKUPINTENT_OPEN;

   if(this->getIntentFlags() & LOOKUPINTENTMSG_FLAG_REVALIDATE)
      return MetaOpCounter_LOOKUPINTENT_REVALIDATE;

   if(this->getIntentFlags() & LOOKUPINTENTMSG_FLAG_STAT)
      return MetaOpCounter_LOOKUPINTENT_STAT;

   return MetaOpCounter_LOOKUPINTENT_SIMPLE;

}
