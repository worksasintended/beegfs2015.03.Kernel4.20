#include "GetQuotaInfoWork.h"
#include <common/net/message/storage/quota/GetQuotaInfoRespMsg.h>
#include <common/toolkit/MessagingTk.h>


/**
 * merges a given list into the map of this worker
 *
 * @param inList the list with the QuotaData to merge with the QuotaDataMap of this worker
 */
void GetQuotaInfoWork::mergeOrInsertNewQuotaData(QuotaDataList* inList)
{
   SafeMutexLock lock = SafeMutexLock(this->quotaResultsMutex);         // L O C K

   for(QuotaDataListIter inListIter = inList->begin(); inListIter != inList->end(); inListIter++)
   {
      QuotaDataMapIter outIter = this->quotaResults->find(inListIter->getID() );
      if(outIter != this->quotaResults->end() )
      {
         if(outIter->second.mergeQuotaDataCounter(&(*inListIter) ) )
         { // success
            *this->result = 0;
         }
         else
         { // insert failed, error value 1 or TargetNumID depends on target selection mode
            if(this->cfg.cfgTargetSelection != GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST)
               *this->result = this->cfg.cfgTargetNumID;
            else
               *this->result = 1;
         }
      }
      else
      {
         if(this->quotaResults->insert(QuotaDataMapVal(inListIter->getID(), *inListIter) ).second)
         { // success
            *this->result = 0;
         }
         else
         { // insert failed, error value 1 or TargetNumID depends on target selection mode
            if(this->cfg.cfgTargetSelection != GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST)
               *this->result = this->cfg.cfgTargetNumID;
            else
               *this->result = 1;
         }
      }
   }

   lock.unlock();                                                       // U N L O C K

   if(*this->result != 0)
   {
      LogContext("GetQuotaInfo").log(Log_WARNING, "Invalid QuotaData from target: " +
        StringTk::uintToStr(this->cfg.cfgTargetNumID));
   }
}

void GetQuotaInfoWork::process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen)
{
   QuotaDataList results;
   bool commRes = false;
   char* respBuf = NULL;

   GetQuotaInfoMsg msg(this->cfg.cfgType);
   prepareMessage(this->messageNumber, &msg);

   NetMessage* respMsg = NULL;
   GetQuotaInfoRespMsg* respMsgCast;

   // request/response
   commRes = MessagingTk::requestResponse(storageNode, &msg, NETMSGTYPE_GetQuotaInfoResp,
      &respBuf, &respMsg);
   if(!commRes)
   {
      // error value 1 or TargetNumID depends on target selection mode
      if(this->cfg.cfgTargetSelection != GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST)
         *this->result = this->cfg.cfgTargetNumID;
      else
         *this->result = 1;

      this->counter->incCount();

      SAFE_DELETE(respMsg);
      SAFE_FREE(respBuf);

      return;
   }

   respMsgCast = (GetQuotaInfoRespMsg*)respMsg;
   respMsgCast->parseQuotaDataList(&results);

   //merge the QuotaData from the response with the existing quotaData

   mergeOrInsertNewQuotaData(&results);

   this->counter->incCount();

   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);
}

/*
 * type dependent message preparations
 *
 * @param messageNumber the sequence number of the message
 * @param msg the message to send
 */
void GetQuotaInfoWork::prepareMessage(int messageNumber, GetQuotaInfoMsg* msg)
{
   if(this->cfg.cfgTargetSelection == GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST)
      msg->setTargetSelectionAllTargetsOneRequestForAllTargets();
   else
   if(this->cfg.cfgTargetSelection == GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST_PER_TARGET)
      msg->setTargetSelectionAllTargetsOneRequestPerTarget(this->cfg.cfgTargetNumID);
   else
   if(this->cfg.cfgTargetSelection == GETQUOTACONFIG_SINGLE_TARGET)
      msg->setTargetSelectionSingleTarget(this->cfg.cfgTargetNumID);

   if(this->cfg.cfgUseList || this->cfg.cfgUseAll)
   {
      msg->setQueryType(GetQuotaInfo_QUERY_TYPE_ID_LIST);
      getIDsFromListForMessage(messageNumber, msg->getIDList());
   }
   else
   if(this->cfg.cfgUseRange)
   {
      unsigned startRange, endRange;
      getIDRangeForMessage(messageNumber, startRange, endRange);
      msg->setIDRange(startRange, endRange);
   }
   else
   {
      msg->setID(this->cfg.cfgID);
   }
}

/*
 * calculates the first ID and the last ID from the ID range, which fits into the payload of the msg
 *
 *  @param messageNumber the sequence number of the message
 *  @param outFirstID the first ID of the range for the message with the given sequence number
 *  @param outLastID the last ID of the range for the message with the given sequence number
 */
void GetQuotaInfoWork::getIDRangeForMessage(int messageNumber, unsigned &outFirstID,
   unsigned &outLastID)
{
   if( (this->cfg.cfgIDRangeEnd - this->cfg.cfgIDRangeStart) <= GETQUOTAINFORESPMSG_MAX_ID_COUNT)
   {
      outFirstID = this->cfg.cfgIDRangeStart;
      outLastID = this->cfg.cfgIDRangeEnd;
   }
   else
   {
      unsigned startThisMessage = this->cfg.cfgIDRangeStart +
         (messageNumber * GETQUOTAINFORESPMSG_MAX_ID_COUNT);
      unsigned startNextMessage = this->cfg.cfgIDRangeStart +
         ( (messageNumber + 1) * GETQUOTAINFORESPMSG_MAX_ID_COUNT);

      outFirstID = startThisMessage;
      outLastID = startNextMessage - 1;

      if(outLastID > this->cfg.cfgIDRangeEnd)
         outLastID = this->cfg.cfgIDRangeEnd;
   }
}

/*
 * calculates the sublist of ID list, which fits into the payload of the msg
 *
 * @param messageNumber the sequence number of the message
 * @param outList the ID list for the message with the given sequence number
 */
void GetQuotaInfoWork::getIDsFromListForMessage(int messageNumber, UIntList* outList)
{
   int counter = 0;
   int startThisMessage = (messageNumber * GETQUOTAINFORESPMSG_MAX_ID_COUNT);
   int startNextMessage = ( (messageNumber + 1) * GETQUOTAINFORESPMSG_MAX_ID_COUNT);

   for(UIntListIter iter = this->cfg.cfgIDList.begin(); iter != this->cfg.cfgIDList.end(); iter++)
   {
      if(counter < startThisMessage)
      {
         counter++;
         continue;
      }
      else
      if(counter >= startNextMessage)
         return;

      counter++;
      outList->push_back(*iter);
   }

}
