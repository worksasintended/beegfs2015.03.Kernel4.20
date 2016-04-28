#include <common/storage/EntryInfo.h>
#include <program/Program.h>
#include <storage/MetaStore.h>
#include "MsgHelperXAttr.h"

const std::string MsgHelperXAttr::XATTR_PREFIX = std::string("user.bgXA.");
const std::string MsgHelperXAttr::CURRENT_DIR_FILENAME = std::string(".");

FhgfsOpsErr MsgHelperXAttr::listxattr(EntryInfo* entryInfo, StringVector& outNames)
{
   FhgfsOpsErr retVal;
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   if (entryInfo->getEntryType() == DirEntryType_DIRECTORY)
   {
      const std::string dirEntryID(entryInfo->getEntryID() );

      DirInode* dir = metaStore->referenceDir(dirEntryID, false );
      if (unlikely(!dir) )
         return FhgfsOpsErr_INTERNAL;

      retVal = dir->listXAttr(CURRENT_DIR_FILENAME, outNames);

      metaStore->releaseDir(dirEntryID);
   }
   else
   {
      const std::string parentID = entryInfo->getParentEntryID();
      const std::string fileName = entryInfo->getFileName();

      DirInode* dir = metaStore->referenceDir(parentID, false);
      if (unlikely(!dir) )
         return FhgfsOpsErr_INTERNAL;

      retVal = dir->listXAttr(fileName, outNames);

      metaStore->releaseDir(parentID);
   }

   if (retVal != FhgfsOpsErr_SUCCESS)
      return retVal;

   // filter list for xattr prefixes, remove prefixes from attributes
   for (size_t i = 0; i < outNames.size(); i++)
   {
      if(outNames[i].compare(0, XATTR_PREFIX.length(), XATTR_PREFIX) == 0)
         outNames[i].erase(0, XATTR_PREFIX.length());
      else
         outNames.erase(outNames.begin() + (i--));
   }

   return FhgfsOpsErr_SUCCESS;
}


FhgfsOpsErr MsgHelperXAttr::getxattr(EntryInfo* entryInfo, const std::string& name,
   CharVector& outValue, ssize_t& inOutSize)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();
   FhgfsOpsErr retVal;

   if (entryInfo->getEntryType() == DirEntryType_DIRECTORY)
   {
      const std::string dirEntryID(entryInfo->getEntryID() );

      DirInode* dir = metaStore->referenceDir(dirEntryID, false);
      if (unlikely(!dir) )
         return FhgfsOpsErr_INTERNAL;

      retVal = dir->getXAttr(CURRENT_DIR_FILENAME, XATTR_PREFIX + name, outValue, inOutSize);

      metaStore->releaseDir(dirEntryID);
   }
   else
   {
      const std::string parentID = entryInfo->getParentEntryID();
      const std::string fileName = entryInfo->getFileName();

      DirInode* dir = metaStore->referenceDir(parentID, false);
      if (unlikely(!dir) )
         return FhgfsOpsErr_INTERNAL;

      retVal = dir->getXAttr(fileName, XATTR_PREFIX + name, outValue, inOutSize);

      metaStore->releaseDir(parentID);
   }

   if (inOutSize > 60*1024) // Attribute might be too large for NetMessage.
   {
      // Note: This can happen if it was set with an older version of the client which did not
      //       include the size check.
      retVal = FhgfsOpsErr_INTERNAL;
      inOutSize = 0;
      outValue.clear();
   }

   return retVal;
}

FhgfsOpsErr MsgHelperXAttr::removexattr(EntryInfo* entryInfo, const std::string& name)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();
   FhgfsOpsErr retVal;

   if (entryInfo->getEntryType() == DirEntryType_DIRECTORY)
   {
      const std::string dirEntryID(entryInfo->getEntryID() );

      DirInode* dir = metaStore->referenceDir(dirEntryID, false);
      if (unlikely(!dir) )
         return FhgfsOpsErr_INTERNAL;

      retVal = dir->removeXAttr(CURRENT_DIR_FILENAME, XATTR_PREFIX + name);

      metaStore->releaseDir(dirEntryID);
   }
   else
   {
      const std::string parentID = entryInfo->getParentEntryID();
      const std::string fileName = entryInfo->getFileName();

      DirInode* dir = metaStore->referenceDir(parentID, false);
      if (unlikely(!dir) )
         return FhgfsOpsErr_INTERNAL;

      retVal = dir->removeXAttr(fileName, XATTR_PREFIX + name);

      metaStore->releaseDir(parentID);
   }

   return retVal;
}

FhgfsOpsErr MsgHelperXAttr::setxattr(EntryInfo* entryInfo, const std::string& name,
   const CharVector& value, int flags)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();
   FhgfsOpsErr retVal;

   if (entryInfo->getEntryType() == DirEntryType_DIRECTORY)
   {
      const std::string dirEntryID(entryInfo->getEntryID() );

      DirInode* dir = metaStore->referenceDir(dirEntryID, false);
      if (unlikely(!dir) )
         return FhgfsOpsErr_INTERNAL;

      retVal = dir->setXAttr(CURRENT_DIR_FILENAME, XATTR_PREFIX + name, value, flags);

      metaStore->releaseDir(dirEntryID);
   }
   else
   {
      const std::string parentID = entryInfo->getParentEntryID();
      const std::string fileName = entryInfo->getFileName();

      DirInode* dir = metaStore->referenceDir(parentID, false);
      if (unlikely(!dir) )
         return FhgfsOpsErr_INTERNAL;

      retVal = dir->setXAttr(fileName, XATTR_PREFIX + name, value, flags);

      metaStore->releaseDir(parentID);
   }

   return retVal;
}
