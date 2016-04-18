#include <common/net/message/storage/mirroring/SetMetadataMirroringRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include <storage/DirInode.h>
#include <storage/MetaStore.h>
#include "SetMetadataMirroringMsgEx.h"


bool SetMetadataMirroringMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("SetMetadataMirroringMsgEx incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, "Received a SetMetadataMirroringMsg from: " + peer);

   EntryInfo* entryInfo = this->getEntryInfo();

   LOG_DEBUG("SetMetadataMirroringMsgEx::processIncoming", Log_SPAM,
      "parentEntryID: " + entryInfo->getParentEntryID() + "; "
      "entryID: " + entryInfo->getEntryID() + "; "
      "name: "  + entryInfo->getFileName() );
   
   FhgfsOpsErr setRes = setDirMirroring(entryInfo);

   
   SetMetadataMirroringRespMsg respMsg(setRes);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   
   
   return true;      
}


FhgfsOpsErr SetMetadataMirroringMsgEx::setDirMirroring(EntryInfo* entryInfo)
{
   const char* logContext = "SetMetadataMirroringMsg (set dir mirror)";

   App* app = Program::getApp();
   MetaStore* metaStore = app->getMetaStore();
   NodeCapacityPools* metaCapacityPools = app->getMetaCapacityPools();


   // verify owner of root dir
   if( (entryInfo->getEntryID() == META_ROOTDIR_ID_STR) &&
       (app->getLocalNodeNumID() != app->getRootDir()->getOwnerNodeID() ) )
   {
      LogContext(logContext).log(Log_DEBUG, "This node does not own the root directory.");
      return FhgfsOpsErr_NOTOWNER;
   }

   DirInode* dir = metaStore->referenceDir(entryInfo->getEntryID(), true);
   if(unlikely(!dir) )
      return FhgfsOpsErr_NOTADIR;

   // choose new dir mirror node
   /* (note: choosing 2 nodes here is a workaround to ensure that we have at least one node
      available that is not the current node.) */

   UInt16Vector mirrorCandidates;

   unsigned numDesiredTargets = 2;
   unsigned minNumRequiredTargets = numDesiredTargets;

   metaCapacityPools->chooseStorageTargets(
      numDesiredTargets, minNumRequiredTargets, NULL, &mirrorCandidates);

   if(unlikely(mirrorCandidates.size() < minNumRequiredTargets) )
   { // (might be caused by a bad list of preferred targets)
      /* note: if we only got one here, that would be ourselve, so we definitely need
         mirrorCandidates.size()==2 */

      LogContext(logContext).logErr(
         "No metadata servers available to mirror directory. "
         "DirID: " + entryInfo->getEntryID() );

      metaStore->releaseDir(entryInfo->getEntryID() );
      return FhgfsOpsErr_UNKNOWNNODE;
   }


   // make sure we don't pick ourselve as mirror

   uint16_t mirrorNodeID;

   if(mirrorCandidates[0] != app->getLocalNodeNumID() )
      mirrorNodeID = mirrorCandidates[0];
   else
      mirrorNodeID = mirrorCandidates[1];


   // set and store new mirror (this also sends the initial copy to the mirror)

   FhgfsOpsErr setRes = dir->setAndStoreMirrorNodeIDChecked(mirrorNodeID);
   if(setRes != FhgfsOpsErr_SUCCESS)
   {
      if(setRes == FhgfsOpsErr_INVAL)
      { // this dir already has a mirror node
         LogContext(logContext).log(Log_DEBUG, "Directory already has a mirror. "
            "DirID: " + entryInfo->getEntryID() );
      }
      else
         LogContext(logContext).logErr("Setting dir mirror failed. "
         "DirID: " + entryInfo->getEntryID() + "; "
         "Error: " + FhgfsOpsErrTk::toErrString(setRes) );
   }


   metaStore->releaseDir(entryInfo->getEntryID() );
   
   return setRes;
}
