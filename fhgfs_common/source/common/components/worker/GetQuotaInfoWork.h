#ifndef GETQUOTAINFOWORK_H_
#define GETQUOTAINFOWORK_H_


#include <common/Common.h>
#include <common/net/message/storage/quota/GetQuotaInfoMsg.h>
#include <common/nodes/Node.h>
#include <common/storage/quota/QuotaData.h>
#include <common/storage/quota/GetQuotaInfo.h>
#include <common/toolkit/SynchronizedCounter.h>


#include "Work.h"


class GetQuotaInfoWork: public Work
{
   public:
      /*
       * Constructor for thread-safe use of the result map
       */
      GetQuotaInfoWork(GetQuotaInfoConfig cfg, Node* storageNode, int messageNumber,
         QuotaDataMap* outQuotaResults, Mutex* quotaResultsMutex,
         SynchronizedCounter *counter, uint16_t* result)
      {
         this->cfg = cfg;
         this->storageNode = storageNode;
         this->messageNumber = messageNumber;
         this->quotaResults = outQuotaResults;
         this->quotaResultsMutex = quotaResultsMutex;

         this->counter = counter;
         this->result = result;
      }

      virtual ~GetQuotaInfoWork()
      {
      }

      virtual void process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);

   private:
      GetQuotaInfoConfig cfg;       // configuration qith all information to query the quota data
      Node* storageNode;            // the node query
      int messageNumber;            // the

      QuotaDataMap* quotaResults;   // the quota data from the server after requesting the server
      Mutex* quotaResultsMutex;     // synchronize quotaResultsMutex

      SynchronizedCounter* counter; // counter for finished worker
      uint16_t* result;             // result of the worker, 0 if success, if error the TargetNumID

      void prepareMessage(int messageNumber, GetQuotaInfoMsg* msg);
      void getIDRangeForMessage(int messageNumber, unsigned &outFirstID, unsigned &outLastID);
      void getIDsFromListForMessage(int messageNumber, UIntList* outList);
      void mergeOrInsertNewQuotaData(QuotaDataList* inList);
};

#endif /* GETQUOTAINFOWORK_H_ */
