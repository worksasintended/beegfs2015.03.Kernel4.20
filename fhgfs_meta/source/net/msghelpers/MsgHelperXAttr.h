#ifndef MSGHELPERXATTR_H_
#define MSGHELPERXATTR_H_

#include <common/storage/StorageErrors.h>

class EntryInfo; // forward declaration
class Socket;

class MsgHelperXAttr
{
   public:
      static FhgfsOpsErr listxattr(EntryInfo* entryInfo, StringVector& outNames);
      static FhgfsOpsErr getxattr(EntryInfo* entryInfo, const std::string& name,
         CharVector& outValue, ssize_t& inOutSize);
      static FhgfsOpsErr removexattr(EntryInfo* entryInfo, const std::string& name);
      static FhgfsOpsErr setxattr(EntryInfo* entryInfo, const std::string& name,
         const CharVector& value, int flags);

      static void resetXAttrFn(void* context);
      static FhgfsOpsErr streamXAttrFn(void* context, std::string& name, CharVector& value);

      static const std::string CURRENT_DIR_FILENAME;
      static const ssize_t MAX_VALUE_SIZE;

      struct StreamXAttrState
      {
         Socket* socket;
         EntryInfo* entryInfo;
         StringVector* names;

         unsigned index;
      };

   private:
      static const std::string XATTR_PREFIX;
};

#endif /*MSGHELPERXATTR_H_*/
