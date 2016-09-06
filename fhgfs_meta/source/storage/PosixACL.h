#ifndef POSIXACL_H_
#define POSIXACL_H_

#include <common/toolkit/serialization/Serialization.h>
#include <common/Common.h>

#include <stdint.h>
#include <vector>

struct ACLEntry
{
   // Definitions to match linux/posix_acl.h and linux/posix_acl_xattr.h
   static const int32_t POSIX_ACL_XATTR_VERSION = 0x0002;
   enum Tag {
      ACL_USER_OBJ  = 0x01,
      ACL_USER      = 0x02,
      ACL_GROUP_OBJ = 0x04,
      ACL_GROUP     = 0x08,
      ACL_MASK      = 0x10,
      ACL_OTHER     = 0x20
   };

   short tag;
   unsigned short perm;
   unsigned id; // uid or gid depending on the tag
};

typedef std::vector<ACLEntry> ACLEntryVec;
typedef ACLEntryVec::const_iterator ACLEntryVecCIter;
typedef ACLEntryVec::iterator ACLEntryVecIter;

class PosixACL
{
   public:
      bool deserializeXAttr(const CharVector& xattr);
      unsigned serialLenXAttr();
      void serializeXAttr(char* buf);

      std::string toString();
      FhgfsOpsErr modifyModeBits(int& mode, bool& needsACL);

      static const std::string defaultACLXAttrName;
      static const std::string accessACLXAttrName;

   private:
      ACLEntryVec entries;

      static bool deserializeACLEntry(const char* buf, size_t bufLen,
         ACLEntry& outEntry, unsigned& outLen);
      static size_t serializeACLEntry(char* buf, const ACLEntry& entry);

   /* inliners */
   public:
      bool empty() { return entries.empty(); }

   private:

      static unsigned serialLenACLEntry()
      {
         return Serialization::serialLenShort() + // tag
                Serialization::serialLenShort() + // perm
                Serialization::serialLenInt();    // id
      }

      enum AclEntryListLenReturn {
         AclListLen_EMPTY = -1,
         AclListLen_TOOSHORT = -2,
         AclListLen_NOTMULTIPLE = -3 };

      static int aclEntryListLen(unsigned bufLen)
      {
         if (bufLen < Serialization::serialLenInt()) // Too small for header.
            return AclListLen_TOOSHORT;

         bufLen -= Serialization::serialLenInt(); // Subtract header length.

         if (bufLen % serialLenACLEntry())
            return AclListLen_NOTMULTIPLE;

         return bufLen / serialLenACLEntry();
      }
};

#endif /*POSIXACL_H_*/
