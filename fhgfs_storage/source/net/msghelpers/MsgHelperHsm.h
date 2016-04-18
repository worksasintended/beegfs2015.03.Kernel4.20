#ifndef MSGHELPERHSM_H_
#define MSGHELPERHSM_H_

#include <common/Common.h>

class MsgHelperHsm
{
   public:
      
      
   private:
      MsgHelperHsm() {}
      
   public:
      static bool sendArchiveFilesMsg(uint16_t targetID, std::string fileHandleID,
         uint16_t collocationID);

};

#endif /* MSGHELPERHSM_H_ */
