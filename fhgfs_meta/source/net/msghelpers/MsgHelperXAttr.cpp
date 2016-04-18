#include <common/storage/EntryInfo.h>
#include <program/Program.h>
#include <storage/MetaStore.h>
#include "MsgHelperXAttr.h"

const std::string MsgHelperXAttr::XATTR_PREFIX = std::string("user.bgXA.");
const std::string MsgHelperXAttr::CURRENT_DIR_FILENAME = std::string(".");

FhgfsOpsErr MsgHelperXAttr::listxattr(EntryInfo* entryInfo, char* outBuf, ssize_t& inOutSize)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   StringVector fullAttrList;
   StringVector filteredAttrList;
   ssize_t outListSize = 0;
   FhgfsOpsErr retVal;

   if (entryInfo->getEntryType() == DirEntryType_DIRECTORY)
   {
      const std::string dirEntryID(entryInfo->getEntryID() );

      DirInode* dir = metaStore->referenceDir(dirEntryID, false );
      if (unlikely(!dir) )
         return FhgfsOpsErr_INTERNAL;

      retVal = dir->listXAttr(CURRENT_DIR_FILENAME, fullAttrList);

      metaStore->releaseDir(dirEntryID);
   }
   else
   {
      const std::string parentID = entryInfo->getParentEntryID();
      const std::string fileName = entryInfo->getFileName();

      DirInode* dir = metaStore->referenceDir(parentID, false);
      if (unlikely(!dir) )
         return FhgfsOpsErr_INTERNAL;

      retVal = dir->listXAttr(fileName, fullAttrList);

      metaStore->releaseDir(parentID);
   }

   if (retVal != FhgfsOpsErr_SUCCESS)
      return retVal;

   // filter list for xattr prefixes, remove prefixes from attributes
   for(StringVectorConstIter attrIter = fullAttrList.begin(); attrIter != fullAttrList.end();
       ++attrIter)
   {
      const std::string attr = *attrIter;

      if(attr.compare(0, XATTR_PREFIX.length(), XATTR_PREFIX) == 0)
      {
         std::string attrName = attr.substr(XATTR_PREFIX.length() );
         filteredAttrList.push_back(attrName);
         outListSize += attrName.length() + 1;
      }
   }

   if(inOutSize == 0)
   {
      inOutSize = outListSize;
   }
   else
   if(inOutSize >= outListSize)
   {
      inOutSize = outListSize;
      char* bufPos = outBuf;

      for(StringVectorConstIter attrIter = filteredAttrList.begin();
          attrIter != filteredAttrList.end(); ++attrIter)
      {
         std::copy(attrIter->begin(), attrIter->end(), bufPos);
         bufPos += attrIter->length();
         *(bufPos++) = '\0'; // add null terminator / list separator
      }
   }
   else
   {
      retVal = FhgfsOpsErr_RANGE;
   }

   return retVal;
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
