#include "PosixACL.h"

const std::string PosixACL::defaultACLXAttrName = "user.bgXA.system.posix_acl_default";
const std::string PosixACL::accessACLXAttrName = "user.bgXA.system.posix_acl_access";

/**
 * @param xattr serialized form of the posix ACL to fill the PosixACL from.
 */
bool PosixACL::deserializeXAttr(const CharVector& xattr)
{
   const char* buf = &xattr.front();
   const unsigned bufLen = xattr.size();
   unsigned bufPos = 0;

   { // version
      unsigned versionBufLen;
      int32_t version;

      if ( !Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
            &version, &versionBufLen) )
         return false;
      bufPos += versionBufLen;

      if (version != ACLEntry::POSIX_ACL_XATTR_VERSION)
         return false;
   }

   int listLen = aclEntryListLen(bufLen);
   if (listLen < 0)
      return false;

   entries.clear();
   entries.reserve(listLen);

   for (int entryNum = 0; entryNum < listLen; entryNum++)
   {
      ACLEntry newEntry;

      unsigned entryBufLen;
      if ( !deserializeACLEntry(&buf[bufPos], bufLen-bufPos, newEntry, entryBufLen) )
         return false;

      bufPos += entryBufLen;

      entries.push_back(newEntry);
   }

   return true;
}

/**
 * Deserialize a single ACL entry.
 */
bool PosixACL::deserializeACLEntry(const char* buf, size_t bufLen,
   ACLEntry& outEntry, unsigned& outLen)
{
   size_t bufPos = 0;

   { // tag
      unsigned tagBufLen;
      if ( !Serialization::deserializeShort(&buf[bufPos], bufLen-bufPos,
            &outEntry.tag, &tagBufLen) )
         return false;
      bufPos += tagBufLen;
   }

   { // perm
      unsigned permBufLen;
      if ( !Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
            &outEntry.perm, &permBufLen) )
         return false;
      bufPos += permBufLen;
   }

   { // id
      unsigned idBufLen;
      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
            &outEntry.id, &idBufLen) )
         return false;
      bufPos += idBufLen;
   }

   outLen = serialLenACLEntry();

   return true;
}

/**
 * Serialize a single ACL entry.
 */
size_t PosixACL::serializeACLEntry(char* buf, const ACLEntry& entry)
{
   size_t bufPos = 0;

   bufPos += Serialization::serializeShort(&buf[bufPos], entry.tag);
   bufPos += Serialization::serializeUShort(&buf[bufPos], entry.perm);
   bufPos += Serialization::serializeUInt(&buf[bufPos], entry.id);

   return bufPos;
}

/**
 * Length of the serizized form of the ACL.
 */
unsigned PosixACL::serialLenXAttr()
{
   return Serialization::serialLenInt() +       // version header
          entries.size() * serialLenACLEntry(); // entries
}

/**
 * Serialize the ACL in posix_acl_xattr form.
 */
void PosixACL::serializeXAttr(char* buf)
{
   size_t bufPos = 0;

   // version header
   bufPos += Serialization::serializeInt(&buf[bufPos], ACLEntry::POSIX_ACL_XATTR_VERSION);

   // entries
   for (ACLEntryVecCIter entryIt = entries.begin(); entryIt != entries.end(); ++entryIt)
      bufPos += serializeACLEntry(&buf[bufPos], *entryIt);
}

/**
 * Modify the ACL based on the given mode bits. Also modifies the mode bits according to the
 * permissions granted in the ACL.
 * This effectively turns a directory default ACL into a file access ACL.
 *
 * @param mode Mode bits of the new file.
 * @returns whether an ACL is necessary for the newly created file.
 */
FhgfsOpsErr PosixACL::modifyModeBits(int& mode, bool& needsACL)
{
   needsACL = false;
   int newMode = mode;

   // Pointers to the group/mask mode of the ACL. In case we find group/mask entries, we have to
   // take them into account last.
   unsigned short* groupPerm = NULL;
   unsigned short* maskPerm = NULL;

   for (ACLEntryVecIter entryIt = entries.begin(); entryIt != entries.end(); ++entryIt)
   {
      ACLEntry& entry = *entryIt;

      switch (entry.tag)
      {
         case ACLEntry::ACL_USER_OBJ:
            {
               // Apply 'user' permission of the mode to the ACL's 'owner' entry.
               entry.perm &= (newMode >> 6) | ~0007;

               // Apply 'owner' entry of the ACL to the owner's permission in the mode flags.
               newMode &= (entry.perm << 6) | ~0700;
            }
            break;

         case ACLEntry::ACL_USER:
         case ACLEntry::ACL_GROUP:
            {
               // If the ACL has named user/group entries, it can't be represented using only
               // mode bits.
               needsACL = true;
            }
            break;

         case ACLEntry::ACL_GROUP_OBJ:
            {
               groupPerm = &entry.perm;
            }
            break;

         case ACLEntry::ACL_OTHER:
            {
               // Apply 'other' permission from the mode to the ACL's 'other' entry.
               entry.perm &= newMode | ~0007;

               // Apply 'other' entry of the ACL to the 'other' permission in the mode flags.
               newMode &= entry.perm | ~0007;
            }
            break;

         case ACLEntry::ACL_MASK:
            {
               maskPerm = &entry.perm;
            }
            break;

         default:
            return FhgfsOpsErr_INTERNAL;
      }
   }

   if (maskPerm)
   {
      // The 'mask' entry of the ACL influences the 'group' access bits and vice-versa.
      *maskPerm &= (newMode >> 3) | ~0007;
      newMode &= (*maskPerm << 3) | ~0070;
   }
   else
   {
      // If there's no mask, the group mode bits are determined using the group acl entry.
      if (!groupPerm)
         return FhgfsOpsErr_INTERNAL;

      *groupPerm &= (newMode >> 3) | ~0007;
      newMode &= (*groupPerm << 3) | ~0070;
   }

   // Apply changes to the last nine mode bits.
   mode = (mode & ~0777) | newMode;

   return FhgfsOpsErr_SUCCESS;
}

std::string PosixACL::toString()
{
   std::ostringstream ostr;

   ostr << "ACL Size: " << entries.size() << std::endl;

   for (ACLEntryVecCIter it = entries.begin(); it != entries.end(); ++it)
      ostr << "Entry[ "
           << "tag: " << it->tag << " perm: " << it->perm << " id: " << it->id
           << " ]" << std::endl;

   return ostr.str();
}
