#ifndef MSGHELPERXATTR_H_
#define MSGHELPERXATTR_H_

#include <common/storage/StorageErrors.h>

class EntryInfo; // forward declaration

class MsgHelperXAttr
{
   public:
      static FhgfsOpsErr listxattr(EntryInfo* entryInfo, char* outBuf, ssize_t& inOutSize);
      static FhgfsOpsErr getxattr(EntryInfo* entryInfo, const std::string& name,
         CharVector& outValue, ssize_t& inOutSize);
      static FhgfsOpsErr removexattr(EntryInfo* entryInfo, const std::string& name);
      static FhgfsOpsErr setxattr(EntryInfo* entryInfo, const std::string& name,
         const CharVector& value, int flags);

      static const std::string CURRENT_DIR_FILENAME;

   private:
      static const std::string XATTR_PREFIX;
};

#endif /*MSGHELPERXATTR_H_*/
