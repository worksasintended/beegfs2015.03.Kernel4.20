#include <program/Program.h>

#include <common/storage/HsmFileMetaData.h>
#include "GamSetCollocationIDMsgEx.h"


bool GamSetCollocationIDMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GamSetCollocationIDMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a GamSetCollocationIDMsg from: ") + peer);
   
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   StringVector dirEntryIDs;
   HsmCollocationIDVector hsmCollocationIDs;
   StringVector failedDirEntryIDs;

   this->parseDirEntryIDs(&dirEntryIDs);
   this->parseCollocationIDs(&hsmCollocationIDs);

   if ( hsmCollocationIDs.size() == 1 ) // only one collocationID given => use it for all entryIDs
      hsmCollocationIDs.insert(hsmCollocationIDs.end(), dirEntryIDs.size() - 1,
         hsmCollocationIDs.front());

   if ( dirEntryIDs.size() != hsmCollocationIDs.size() )
   {
      log.logErr("Request to set GAM collocation IDs, but size of collocation IDs vector does not "
         "match.");
      failedDirEntryIDs = dirEntryIDs;
      goto send_response;
   }

   {
      StringVectorIter entryIDIter = dirEntryIDs.begin();
      HsmCollocationIDVectorIter collocationIDIter = hsmCollocationIDs.begin();

      for ( ; entryIDIter != dirEntryIDs.end(); entryIDIter++, collocationIDIter++ )
      {
         std::string dirID = *entryIDIter;
         HsmCollocationID hsmCollocationID = *collocationIDIter;

         DirInode *dirInode = metaStore->referenceDir(dirID, true);

         if ( !dirInode )
         {
            log.logErr(
               "Request to set GAM collocation ID for directory with entryID " + dirID
                  + ", but entryID does not exist or is not a directory.");

            failedDirEntryIDs.push_back(dirID);
            continue; // try next
         }

         dirInode->setHsmCollocationID(hsmCollocationID);

         // set collocation ID for ALL children in there
         StringList fileList;

         unsigned maxOutNames = 1000;
         int64_t offset = 0;
         unsigned resultSize = 0;
         do
         {
            StringList tmpFiles;
            int64_t outOffset;
            dirInode->listIncremental(offset, maxOutNames, &tmpFiles, &outOffset);
            offset = outOffset;
            resultSize = tmpFiles.size();
            fileList.splice(fileList.begin(), tmpFiles);
         } while ( resultSize == maxOutNames );


         for ( StringListIter fileListIter = fileList.begin(); fileListIter != fileList.end();
            fileListIter++ )
         {
            EntryInfo entryInfo;
            dirInode->getEntryInfo(*fileListIter, entryInfo);

            if ( DirEntryType_ISREGULARFILE(entryInfo.getEntryType()))
            {
               FileInode* childFileInode = metaStore->referenceFile(&entryInfo);
               if ( childFileInode )
               {
                  childFileInode->setAndStoreHsmCollocationID(&entryInfo, hsmCollocationID);
                  metaStore->releaseFile(entryInfo.getParentEntryID(), childFileInode);
               }
            }
            else
            if ( DirEntryType_ISDIR(entryInfo.getEntryType()))
            {
               DirInode* childDirInode = metaStore->referenceDir(entryInfo.getEntryID(), true);
               if ( childDirInode )
               {
                  childDirInode->setAndStoreHsmCollocationID(hsmCollocationID);
                  metaStore->releaseDir(entryInfo.getEntryID());
               }
            }
         }
         metaStore->releaseDir(dirID);
      }
   }

   // send response
   send_response:
   GamSetCollocationIDRespMsg respMsg(&failedDirEntryIDs);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
      sizeof(struct sockaddr_in));

   return true;
}

