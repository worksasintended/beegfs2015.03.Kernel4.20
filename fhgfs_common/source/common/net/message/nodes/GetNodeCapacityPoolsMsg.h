#ifndef GETNODECAPACITYPOOLSMSG_H_
#define GETNODECAPACITYPOOLSMSG_H_

#include <common/Common.h>
#include "../SimpleIntMsg.h"


enum CapacityPoolQueryType
{
   CapacityPoolQuery_META = 0,
   CapacityPoolQuery_STORAGE = 1,
   CapacityPoolQuery_STORAGEBUDDIES = 2
};


class GetNodeCapacityPoolsMsg : public SimpleIntMsg
{
   public:
      GetNodeCapacityPoolsMsg(CapacityPoolQueryType poolType) :
         SimpleIntMsg(NETMSGTYPE_GetNodeCapacityPools, poolType)
      {
      }

      /**
       * For deserialization only.
       */
      GetNodeCapacityPoolsMsg() : SimpleIntMsg(NETMSGTYPE_GetNodeCapacityPools)
      {
      }


   private:


   public:
      // getters & setters

      CapacityPoolQueryType getCapacityPoolQueryType()
      {
         return (CapacityPoolQueryType)getValue();
      }
};

#endif /* GETNODECAPACITYPOOLSMSG_H_ */
