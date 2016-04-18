#include "GetEntryInfoWork.h"
#include <program/Program.h>

void GetEntryInfoWork::process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen)
{
      log.log(5, std::string("Requesting Entry info for path: ") + pathStr );

      // find the owner node of the path
      Path path(pathStr);
      path.setAbsolute(true);
      
      Node* ownerNode = NULL;
      EntryInfo entryInfo;
      
      FhgfsOpsErr findRes = MetadataTk::referenceOwner(&path, false, metaNodes, &ownerNode,
         &entryInfo);
      if(findRes != FhgfsOpsErr_SUCCESS)
      {
         log.logErr("Unable to find metadata node for path: " + pathStr);
         log.logErr("Error: " + std::string(FhgfsOpsErrTk::toErrString(findRes)));
      }
      else
      {
         bool commRes;
         char* respBuf = NULL;
         NetMessage* respMsg = NULL;
         GetEntryInfoRespMsg* respMsgCast;

         GetEntryInfoMsg getInfoMsg(&entryInfo);
         commRes = MessagingTk::requestResponse(
               ownerNode, &getInfoMsg, NETMSGTYPE_GetEntryInfoResp, &respBuf, &respMsg);

         if(commRes) 
         {
            respMsgCast = (GetEntryInfoRespMsg*)respMsg;
            StripePattern* pattern = NULL;
            pattern = respMsgCast->createPattern();
            *outChunkSize = pattern->getChunkSize(); 
            *outDefaultNumTargets = pattern->getDefaultNumTargets();
         }
         else
         {
            log.logErr("Communication failed");
            *outChunkSize = 0;
            *outDefaultNumTargets = 0;
         }
         waitCond->broadcast();
         metaNodes->releaseNode(&ownerNode);
         SAFE_DELETE(respMsg);
         SAFE_FREE(respBuf);    
      }
}
