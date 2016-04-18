/*
 * Dentry and inode serialization/deserialization.
 *
 * Note: Currently inodes and dentries are stored in exactly the same format, even if
 *       inodes are not inlined into a dentry.
 *       If we should add another inode-only format, all code linking inodes to dentries
 *       e.g. (MetaStore::unlinkInodeLaterUnlocked() calling dirInode->linkInodeToDir() must
 *       be updated.
 */

#include <program/Program.h>
#include <common/storage/StorageDefinitions.h>
#include "DiskMetaData.h"
#include "DirInode.h"
#include "FileInode.h"
#include "DirEntry.h"


#define DISKMETADATA_LOG_CONTEXT "DiskMetadata"


// 8-bit
#define DIRENTRY_STORAGE_FORMAT_VER2  2   // version beginning with release 2011.04
#define DIRENTRY_STORAGE_FORMAT_VER3  3   // version, same as V2, but removes the file name
                                          // from the dentry (for dir.dentries)
#define DIRENTRY_STORAGE_FORMAT_VER4  4   // version, which includes inlined inodes, deprecated
#define DIRENTRY_STORAGE_FORMAT_VER5  5   /* version, which includes inlined inodes
                                           * and chunk-path V3, StatData have a flags field */

#define DIRECTORY_STORAGE_FORMAT_VER  1


unsigned DiskMetaData::serializeFileInode(char* buf)
{
   // inodeData set in constructor

   if (!DirEntryType_ISVALID(this->dentryDiskData->getDirEntryType() ) )
   {
      StatData* statData = this->inodeData->getInodeStatData();
      unsigned mode = statData->getMode();
      DirEntryType entryType = MetadataTk::posixFileTypeToDirEntryType(mode);
      this->dentryDiskData->setDirEntryType(entryType);

#ifdef BEEGFS_DEBUG
      const char* logContext = "Serialize FileInode";
      LogContext(logContext).logErr("Bug: entryType not set!");
      LogContext(logContext).logBacktrace();
#endif
   }

   /* We use this method to clone inodes which might be inlined into a dentry, so the real
    * meta type depends on if the inode is inlined or not.  */
   DiskMetaDataType metaDataType;
   if (this->dentryDiskData->dentryFeatureFlags & DENTRY_FEATURE_INODE_INLINE)
      metaDataType = DiskMetaDataType_FILEDENTRY;
   else
      metaDataType = DiskMetaDataType_FILEINODE;

   return serializeInDentryFormat(buf, metaDataType);
}

unsigned DiskMetaData::serializeDentry(char* buf)
{
   DiskMetaDataType metaDataType;

   if (DirEntryType_ISDIR(this->dentryDiskData->getDirEntryType() ) )
      metaDataType = DiskMetaDataType_DIRDENTRY;
   else
      metaDataType = DiskMetaDataType_FILEDENTRY;

   return serializeInDentryFormat(buf, metaDataType);

}

/*
 * Note: Current object state is used for the serialization
 */
unsigned DiskMetaData::serializeInDentryFormat(char* buf, DiskMetaDataType metaDataType)
{
   // note: the total amount of serialized data may not be larger than DIRENTRY_SERBUF_SIZE

   size_t bufPos = 0;
   int dentryFormatVersion;

   // set the type into the entry (1 byte)
   bufPos = Serialization::serializeUInt8(&buf[bufPos], metaDataType);

   // storage-format-version (1 byte)
   if (DirEntryType_ISDIR(this->dentryDiskData->getDirEntryType() ) )
   {
      // metadata format version-3 for directories
      bufPos += Serialization::serializeUInt8(&buf[bufPos], (uint8_t)DIRENTRY_STORAGE_FORMAT_VER3);

      dentryFormatVersion = DIRENTRY_STORAGE_FORMAT_VER3;
   }
   else
   {
      if (this->inodeData->getOrigFeature() == FileInodeOrigFeature_TRUE)
         dentryFormatVersion = DIRENTRY_STORAGE_FORMAT_VER5;
      else
         dentryFormatVersion = DIRENTRY_STORAGE_FORMAT_VER4;

      // metadata format version-4 for files (inlined inodes)
      bufPos += Serialization::serializeUInt8(&buf[bufPos], (uint8_t) dentryFormatVersion);
   }

   // dentry feature flags (2 bytes)
   bufPos += Serialization::serializeUInt16(&buf[bufPos],
      this->dentryDiskData->getDentryFeatureFlags() );

   // entryType (1 byte)
   // (note: we have a fixed position for the entryType byte: DIRENTRY_TYPE_BUF_POS)
   bufPos += Serialization::serializeUInt8(&buf[bufPos], (uint8_t)
      this->dentryDiskData->getDirEntryType() );

   // mirrorNodeID (depends on feature flag) + padding
   if(this->dentryDiskData->getDentryFeatureFlags() & DENTRY_FEATURE_MIRRORED)
   { // mirrorNodeID (2 bytes) + padding (1 byte)
      bufPos += Serialization::serializeUInt16(&buf[bufPos],
         this->dentryDiskData->getMirrorNodeID() );

      // 1 byte padding for 8 byte alignment
      memset(&buf[bufPos], 0 , 1);
      bufPos += 1;
   }
   else
   { // 3 bytes padding for 8 byte alignment
      memset(&buf[bufPos], 0 , 3);
      bufPos += 3;
   }

   // end of 8 byte header

   switch (dentryFormatVersion)
   {
      case DIRENTRY_STORAGE_FORMAT_VER3:
      {
         bufPos += serializeDentryV3(&buf[bufPos]); // V3, currently for dirs only
      }; break;
      case DIRENTRY_STORAGE_FORMAT_VER4:
      {
         bufPos += serializeDentryV4(&buf[bufPos]); // V4 for files with inlined inodes
      }; break;
      case DIRENTRY_STORAGE_FORMAT_VER5:
      {
         bufPos += serializeDentryV5(&buf[bufPos]); // inlined inodes + chunk-path-V3
      }; break;
   }

   return bufPos;
}

/*
 * Version 3 format, now only used for directories and for example for disposal files
 *
 * Note: Do not call directly, but call serializeDentry()
 */
unsigned DiskMetaData::serializeDentryV3(char* buf)
{
   size_t bufPos = 0;

   // ID
   std::string entryID = this->dentryDiskData->getEntryID();
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], entryID.size(), entryID.c_str() );

   // ownerNodeID
   bufPos += Serialization::serializeUShort(&buf[bufPos], this->dentryDiskData->getOwnerNodeID() );


   return bufPos;
}

/*
 * Version 4 format, for files with inlined inodes
 *
 * Note: Do not call directly, but call serializeDentry()
 */
unsigned DiskMetaData::serializeDentryV4(char* buf)
{
   StatData* statData           = this->inodeData->getInodeStatData();
   StripePattern* stripePattern = this->inodeData->getPattern();

#ifdef BEEGFS_HSM_DEPRECATED
   HsmFileMetaData* hsmFileMetaData = this->inodeData->getHsmFileMetaData();
#endif

   size_t bufPos = 0;

   // inode feature flags (4 bytes)
   bufPos += Serialization::serializeUInt(&buf[bufPos], inodeData->getInodeFeatureFlags());

   // unused, was the chunkHash
   bufPos += 4;

   // statData (has 64-bit values, so buffer should be 8-bytes aligned)
   bufPos += statData->serialize(false, false, false, &buf[bufPos]);

   // ID
   std::string entryID = this->dentryDiskData->getEntryID();
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], entryID.size(), entryID.c_str() );

   // mirrorNodeID (depends on feature flag)
   if(inodeData->getInodeFeatureFlags() & FILEINODE_FEATURE_MIRRORED)
      bufPos += Serialization::serializeUInt16(&buf[bufPos], inodeData->getMirrorNodeID() );

   // stripePattern
   bufPos += stripePattern->serialize(&buf[bufPos]);

#ifdef BEEGFS_HSM_DEPRECATED
   // HsmFileMetaData
   if (inodeData->getInodeFeatureFlags() & FILEINODE_FEATURE_HSM)
      bufPos +=  hsmFileMetaData->serialize(&buf[bufPos]);
#endif

   return bufPos;
}

/*
 * Version 5 format, for files with inlined inodes and orig-parentID + orig-UID
 *
 * Note: Do not call directly, but call serializeDentry()
 */
unsigned DiskMetaData::serializeDentryV5(char* buf)
{
   StatData* statData           = this->inodeData->getInodeStatData();
   StripePattern* stripePattern = this->inodeData->getPattern();
   unsigned inodeFeatureFlags   = inodeData->getInodeFeatureFlags();

#ifdef BEEGFS_HSM_DEPRECATED
   HsmFileMetaData* hsmFileMetaData = this->inodeData->getHsmFileMetaData();
#endif

   size_t bufPos = 0;

   // inode feature flags (4 bytes)
   bufPos += Serialization::serializeUInt(&buf[bufPos], inodeFeatureFlags);

   bufPos += 4; // unused (4 bytes) for 8 byte alignment

   // statData (has 64-bit values, so buffer should be 8-bytes aligned)
   bufPos += statData->serialize(false, true, false, &buf[bufPos]);

   // orig-UID (4 bytes)
   if (inodeFeatureFlags & FILEINODE_FEATURE_HAS_ORIG_UID)
   {
      bufPos += Serialization::serializeUInt(&buf[bufPos], this->inodeData->getOrigUID() );
   }

   // orig-parentEntryID
   if (inodeFeatureFlags & FILEINODE_FEATURE_HAS_ORIG_PARENTID)
   {
      std::string origParentID = this->inodeData->getOrigParentEntryID();
      bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
         origParentID.size(), origParentID.c_str() );
   }

   // ID
   std::string entryID = this->dentryDiskData->getEntryID();
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], entryID.size(), entryID.c_str() );

   // mirrorNodeID (depends on feature flag)
   if(inodeFeatureFlags & FILEINODE_FEATURE_MIRRORED)
      bufPos += Serialization::serializeUInt16(&buf[bufPos], inodeData->getMirrorNodeID() );

   // stripePattern
   bufPos += stripePattern->serialize(&buf[bufPos]);

#ifdef BEEGFS_HSM_DEPRECATED
   // HsmFileMetaData
   if (inodeData->getInodeFeatureFlags() & FILEINODE_FEATURE_HSM)
      bufPos +=  hsmFileMetaData->serialize(&buf[bufPos]);
#endif

   return bufPos;
}

/**
 * Return the type of an inode and deserialize into the correct structure. This is mostly useful
 * for fhgfs-fsck, which does not know which inode type to expect.
 *
 * TODO Bernd / Christian: Needs to be tested. Probably with cppunit.
 */
DirEntryType DiskMetaData::deserializeInode(const char* buf, FileInode** outFileInode,
   DirInode** outDirInode)
{
   *outFileInode = NULL;
   *outDirInode  = NULL;

   FileInode *fileInode = NULL;
   DirInode * dirInode  = NULL;

   DiskMetaDataType metaDataType;
   DirEntryType entryType = DirEntryType_INVALID;


   { // DiskMetaDataType
      uint8_t type;
      unsigned metaDataTypeLen;

      size_t bufPos = DISKMETADATA_TYPE_BUF_POS;
      size_t bufLen = 1;

      if(!Serialization::deserializeUInt8(&buf[bufPos], bufLen-bufPos, &type,
         &metaDataTypeLen) )
         return DirEntryType_INVALID;

      metaDataType = (DiskMetaDataType) type;
      bufPos += metaDataTypeLen;
   }

   if (metaDataType == DiskMetaDataType_DIRDENTRY) // currently cannot have an inode included
      return DirEntryType_INVALID;

   if (metaDataType == DiskMetaDataType_DIRINODE)
   {
      entryType = DirEntryType_DIRECTORY;
      std::string entryID = ""; // unkown
      dirInode = new DirInode(entryID);

      bool deserialRes = deserializeDirInode(buf, dirInode);
      if (deserialRes == false)
      {
         delete dirInode;
         return DirEntryType_INVALID;
      }
   }
   else
   if ((metaDataType == DiskMetaDataType_FILEINODE) ||
       (metaDataType == DiskMetaDataType_FILEDENTRY) )
   {
      fileInode = new FileInode();

      bool deserializeRes = fileInode->deserializeMetaData(buf);
      if (deserializeRes == false)
      { // deserialization seems to have failed
         delete fileInode;
         return DirEntryType_INVALID;
      }

      entryType = MetadataTk::posixFileTypeToDirEntryType(fileInode->getMode() );

   }

   *outFileInode = fileInode;
   *outDirInode  = dirInode;
   return entryType;
}

/**
 * This method is for file-inodes located in the inode-hash directories.
 */
bool DiskMetaData::deserializeFileInode(const char* buf)
{
   // right now file inodes are stored in the dentry format only, that will probably change
   // later on.
   return deserializeDentry(buf);
}

/*
 * Deserialize a dentry buffer. Here we only deserialize basic values and will continue with
 * version specific dentry sub functions.
 *
 * Note: Applies deserialized data directly to the current object
 */
bool DiskMetaData::deserializeDentry(const char* buf)
{
   const char* logContext = DISKMETADATA_LOG_CONTEXT " (Dentry Deserialization)";
   // note: assumes that the total amount of serialized data may not be larger than
      // DIRENTRY_SERBUF_SIZE

   size_t bufLen = DIRENTRY_SERBUF_SIZE;
   size_t bufPos = 0;
   uint8_t formatVersion; // which dentry format version

   { // DiskMetaDataType
      uint8_t metaDataType;
      unsigned metaDataTypeLen;

      if(!Serialization::deserializeUInt8(&buf[bufPos], bufLen-bufPos, &metaDataType,
         &metaDataTypeLen) )
      {
         std::string serialType = "DiskMetaDataType";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      bufPos += metaDataTypeLen;
   }

   { // metadata format version
      unsigned formatVersionFieldLen;

      if(!Serialization::deserializeUInt8(&buf[bufPos], bufLen-bufPos, &formatVersion,
         &formatVersionFieldLen) )
      {
         std::string serialType = "Format version";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bufPos += formatVersionFieldLen;
   }

   { // dentry feature flags
      unsigned featureFlagsFieldLen;
      uint16_t dentryFeatureFlags;

      if(!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
         &dentryFeatureFlags, &featureFlagsFieldLen) )
      {
         std::string serialType = "Feature flags";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bool compatCheckRes = checkFeatureFlagsCompat(
         dentryFeatureFlags, getSupportedDentryFeatureFlags() );
      if(unlikely(!compatCheckRes) )
      {
         LogContext(logContext).logErr("Incompatible DirEntry feature flags found. "
            "Used flags (hex): " + StringTk::uintToHexStr(dentryFeatureFlags) + "; "
            "Supported flags (hex): " + StringTk::uintToHexStr(getSupportedDentryFeatureFlags() ) );

         return false;
      }

      this->dentryDiskData->setDentryFeatureFlags(dentryFeatureFlags);
      bufPos += featureFlagsFieldLen;
   }

   { // entryType
     // (note: we have a fixed position for the entryType byte: DIRENTRY_TYPE_BUF_POS)
      unsigned typeLen;
      uint8_t type;

      if(!Serialization::deserializeUInt8(&buf[bufPos], bufLen-bufPos, &type, &typeLen) )
      {
         std::string serialType = "entryType";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      this->dentryDiskData->setDirEntryType((DirEntryType)type );
      bufPos += typeLen;
   }

   // mirrorNodeID (depends on feature flag) + padding
   if(this->dentryDiskData->getDentryFeatureFlags() & DENTRY_FEATURE_MIRRORED)
   { // mirrorNodeID + padding
      unsigned mirrorNodeIDFieldLen;
      uint16_t mirrorNodeID;

      if(!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
         &mirrorNodeID, &mirrorNodeIDFieldLen) )
      {
         std::string serialType = "mirrorNodeID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      this->dentryDiskData->setMirrorNodeID(mirrorNodeID);
      bufPos += mirrorNodeIDFieldLen;

      // 1 byte padding for 8 byte aligned header
      bufPos += 1;
   }
   else
   { // 3 bytes padding for 8 byte aligned header
      bufPos += 3;
   }

   // end of 8-byte header

   switch (formatVersion)
   {
      case DIRENTRY_STORAGE_FORMAT_VER3:
      {
         unsigned dentry3Len;

         if (unlikely(!this->deserializeDentryV3(&buf[bufPos], bufLen - bufPos, &dentry3Len) ) )
            return false;

         bufPos += dentry3Len;
      } break;

      case DIRENTRY_STORAGE_FORMAT_VER4:
      {
         // data inlined, so we ourselves
         App* app = Program::getApp();
         this->dentryDiskData->setOwnerNodeID(app->getLocalNode()->getNumID() );
         unsigned dentry4Len;

         if (unlikely(!this->deserializeDentryV4(&buf[bufPos], bufLen - bufPos, &dentry4Len) ) )
            return false;

         bufPos += dentry4Len;

         this->inodeData->setOrigFeature(FileInodeOrigFeature_FALSE); // V4 does not have it
      } break;

      case DIRENTRY_STORAGE_FORMAT_VER5:
      {
         // data inlined, so we ourselves
         App* app = Program::getApp();
         this->dentryDiskData->setOwnerNodeID(app->getLocalNode()->getNumID() );
         unsigned dentry4Len;

         if (unlikely(!this->deserializeDentryV5(&buf[bufPos], bufLen - bufPos, &dentry4Len) ) )
            return false;

         bufPos += dentry4Len;

         this->inodeData->setOrigFeature(FileInodeOrigFeature_TRUE); // V5 has the origFeature

      } break;

      default:
      {
         LogContext(logContext).logErr("Invalid Storage Format: " +
            StringTk::uintToStr((unsigned) formatVersion) );
         return false;
      } break;

   }

   return true;
}

/**
 * Deserialize dentries, which have the V3 format.
 *
 * NOTE: Do not call it directly, but only from DiskMetaData::deserializeDentry()
 *
 */
bool DiskMetaData::deserializeDentryV3(const char* buf, size_t bufLen, unsigned* outLen)
{
   const char* logContext = DISKMETADATA_LOG_CONTEXT " (Dentry Deserialization V3)";
   size_t bufPos = 0;

   { // entryID
      unsigned idBufLen;
      unsigned idLen;
      const char* idStrStart;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &idLen, &idStrStart, &idBufLen) )
      {
         std::string serialType = "entryID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      std::string entryID(idStrStart, idLen);
      this->dentryDiskData->setEntryID(entryID);
      this->inodeData->setEntryID(entryID);

      bufPos += idBufLen;
   }

   { // ownerNodeID
      unsigned ownerBufLen;
      uint16_t ownerNodeID;

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &ownerNodeID, &ownerBufLen) )
      {
         std::string serialType = "ownerNodeID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      this->dentryDiskData->setOwnerNodeID(ownerNodeID);
      bufPos += ownerBufLen;
   }

   *outLen = bufPos;

   return true;
}

/**
 * Deserialize dentries, which have the V4 format, which includes inlined inodes and have the old
 * chunk path (V1, by directly in hash dirs)
 *
 * NOTE: Do not call it directly, but only from DiskMetaData::deserializeDentry()
 *
 */
bool DiskMetaData::deserializeDentryV4(const char* buf, size_t bufLen, unsigned* outLen)
{
   const char* logContext = DISKMETADATA_LOG_CONTEXT " (Dentry Deserialization V4)";
   size_t bufPos = 0;
   unsigned inodeFeatureFlags;

   { // inode feature flags
      unsigned inodeFeatureFlagsLen;

      if (!Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &inodeFeatureFlags,
         &inodeFeatureFlagsLen) )
      {
         std::string serialType = "inode feature flags";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bool compatCheckRes = checkFeatureFlagsCompat(
         inodeFeatureFlags, getSupportedDentryV4FileInodeFeatureFlags() );
      if(unlikely(!compatCheckRes) )
      {
         LogContext(logContext).logErr("Incompatible FileInode feature flags found. "
            "Used flags (hex): " + StringTk::uintToHexStr(inodeFeatureFlags) + "; "
            "Supported (hex): " +
            StringTk::uintToHexStr(getSupportedDentryV4FileInodeFeatureFlags() ) );

         return false;
      }

      this->inodeData->setInodeFeatureFlags(inodeFeatureFlags);

      bufPos += inodeFeatureFlagsLen;

   }

   // unused, was the chunkHash
   bufPos += 4;

   { // statData
      StatData* statData = this->inodeData->getInodeStatData();
      unsigned statDataLen;

      if (!statData->deserialize(false, false, false, &buf[bufPos], bufLen-bufPos, &statDataLen) )
      {
         std::string serialType = "statData";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bufPos += statDataLen;
   }

   // note: up to here only fixed integers length, below follow variable string lengths

   { // ID
      unsigned idBufLen;
      unsigned idLen;
      const char* idStrStart;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &idLen, &idStrStart, &idBufLen) )
      {
         std::string serialType = "entryID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      std::string entryID(idStrStart, idLen);
      this->dentryDiskData->setEntryID(entryID);
      this->inodeData->setEntryID(entryID);
      bufPos += idBufLen;
   }

   // mirrorNodeID (depends on feature flag)
   if(inodeFeatureFlags & FILEINODE_FEATURE_MIRRORED)
   {
      uint16_t mirrorNodeID;
      unsigned mirrorNodeIDLen;

      if (!Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &mirrorNodeID,
         &mirrorNodeIDLen) )
      {
         std::string serialType = "mirrorNodeID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      this->inodeData->setMirrorNodeID(mirrorNodeID);

      bufPos += mirrorNodeIDLen;
   }

   { // stripePattern - not aligned, needs to be below aligned values
      unsigned patternBufLen;
      StripePatternHeader patternHeader;
      const char* patternStart;
      std::string serialType = "stripePattern";

      if(!StripePattern::deserializePatternPreprocess(&buf[bufPos], bufLen-bufPos,
         &patternHeader, &patternStart, &patternBufLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      StripePattern* pattern = StripePattern::createFromBuf(patternStart, patternHeader);
      if (unlikely(pattern->getPatternType() == STRIPEPATTERN_Invalid) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         SAFE_DELETE(pattern);
         return false;
      }

      this->inodeData->setPattern(pattern);
      bufPos += patternBufLen;
   }

#ifdef BEEGFS_HSM_DEPRECATED
   { // HsmFileMetaData - not aligned
      if (inodeFeatureFlags & FILEINODE_FEATURE_HSM )
      {
         unsigned hsmFileMetaDataBufLen;

         HsmFileMetaData* hsmFileMetaData = this->inodeData->getHsmFileMetaData();
         if ( !hsmFileMetaData->deserialize(&buf[bufPos], bufLen - bufPos, &hsmFileMetaDataBufLen) )
            return false;

         bufPos += hsmFileMetaDataBufLen;
      }
   }
#endif

   // sanity checks

   #ifdef BEEGFS_DEBUG
      if (unlikely(!(this->dentryDiskData->getDentryFeatureFlags() & DENTRY_FEATURE_IS_FILEINODE) ))
      {
         LogContext(logContext).logErr("Bug: inode data successfully deserialized, but "
            "the file-inode flag is not set. ");
         return false;
      }
   #endif

   *outLen = bufPos;

   return true;
}

/**
 * Deserialize dentries, which have the V4 format, which includes inlined inodes and have the new
 * chunk path (V2, which has UID and parentID)
 *
 * NOTE: Do not call it directly, but only from DiskMetaData::deserializeDentry()
 *
 */
bool DiskMetaData::deserializeDentryV5(const char* buf, size_t bufLen, unsigned* outLen)
{
   const char* logContext = DISKMETADATA_LOG_CONTEXT " (Dentry Deserialization V5)";
   size_t bufPos = 0;
   unsigned inodeFeatureFlags;

   { // inode feature flags
      unsigned inodeFeatureFlagsLen;

      if (!Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &inodeFeatureFlags,
         &inodeFeatureFlagsLen) )
      {
         std::string serialType = "inode feature flags";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bool compatCheckRes = checkFeatureFlagsCompat(
         inodeFeatureFlags, getSupportedDentryV5FileInodeFeatureFlags() );
      if(unlikely(!compatCheckRes) )
      {
         LogContext(logContext).logErr("Incompatible FileInode feature flags found. "
            "Used flags (hex): " + StringTk::uintToHexStr(inodeFeatureFlags) + "; "
            "Supported (hex): "
            + StringTk::uintToHexStr(getSupportedDentryV5FileInodeFeatureFlags() ) );

         return false;
      }

      this->inodeData->setInodeFeatureFlags(inodeFeatureFlags);

      bufPos += inodeFeatureFlagsLen;
   }

   bufPos += 4; // unused, for alignment

   StatData* statData = this->inodeData->getInodeStatData();
   { // statData
      unsigned statDataLen;

      if (!statData->deserialize(false, true, false, &buf[bufPos], bufLen-bufPos, &statDataLen) )
      {
         std::string serialType = "statData";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bufPos += statDataLen;
   }

   // orig-UID
   if (inodeFeatureFlags & FILEINODE_FEATURE_HAS_ORIG_UID)
   {
      unsigned origParentUID;
      unsigned origParentUIDLen;

      if (!Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &origParentUID,
         &origParentUIDLen) )
      {
         std::string serialType = "origParentUID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      this->inodeData->setOrigUID(origParentUID);

      bufPos += origParentUIDLen;
   }
   else
   {  // no separate field, orig-UID and UID are identical
      unsigned origParentUID = statData->getUserID();
      this->inodeData->setOrigUID(origParentUID);
   }

   // orig-parentEntryID
   if (inodeFeatureFlags & FILEINODE_FEATURE_HAS_ORIG_PARENTID)
   {
      unsigned idBufLen;
      unsigned idLen;
      const char* idStrStart;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &idLen, &idStrStart, &idBufLen) )
      {
         std::string serialType = "origParentEntryID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      std::string origParentEntryID(idStrStart, idLen);
      this->inodeData->setDiskOrigParentEntryID(origParentEntryID);
      bufPos += idBufLen;
   }

   // note: up to here only fixed integers length, below follow variable string lengths

   { // ID
      unsigned idBufLen;
      unsigned idLen;
      const char* idStrStart;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &idLen, &idStrStart, &idBufLen) )
      {
         std::string serialType = "entryID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      std::string entryID(idStrStart, idLen);
      this->dentryDiskData->setEntryID(entryID);
      this->inodeData->setEntryID(entryID);
      bufPos += idBufLen;
   }

   // mirrorNodeID (depends on feature flag)
   if(inodeFeatureFlags & FILEINODE_FEATURE_MIRRORED)
   {
      uint16_t mirrorNodeID;
      unsigned mirrorNodeIDLen;

      if (!Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &mirrorNodeID,
         &mirrorNodeIDLen) )
      {
         std::string serialType = "mirrorNodeID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      this->inodeData->setMirrorNodeID(mirrorNodeID);

      bufPos += mirrorNodeIDLen;
   }

   { // stripePattern - not aligned, needs to be below aligned values
      unsigned patternBufLen;
      StripePatternHeader patternHeader;
      const char* patternStart;
      std::string serialType = "stripePattern";

      if(!StripePattern::deserializePatternPreprocess(&buf[bufPos], bufLen-bufPos,
         &patternHeader, &patternStart, &patternBufLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      StripePattern* pattern = StripePattern::createFromBuf(patternStart, patternHeader);
      if (unlikely(pattern->getPatternType() == STRIPEPATTERN_Invalid) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         SAFE_DELETE(pattern);
         return false;
      }

      this->inodeData->setPattern(pattern);
      bufPos += patternBufLen;
   }

#ifdef BEEGFS_HSM_DEPRECATED
   { // HsmFileMetaData - not aligned
      if (inodeFeatureFlags & FILEINODE_FEATURE_HSM )
      {
         unsigned hsmFileMetaDataBufLen;

         HsmFileMetaData* hsmFileMetaData = this->inodeData->getHsmFileMetaData();
         if ( !hsmFileMetaData->deserialize(&buf[bufPos], bufLen - bufPos, &hsmFileMetaDataBufLen) )
            return false;

         bufPos += hsmFileMetaDataBufLen;
      }
   }
#endif

   // sanity checks

   #ifdef BEEGFS_DEBUG
      if (unlikely(!(this->dentryDiskData->getDentryFeatureFlags() & DENTRY_FEATURE_IS_FILEINODE) ))
      {
         LogContext(logContext).logErr("Bug: inode data successfully deserialized, but "
            "the file-inode flag is not set. ");
         return false;
      }
   #endif

   *outLen = bufPos;

   return true;
}


/*
 * Note: Current object state is used for the serialization
 */
unsigned DiskMetaData::serializeDirInode(char* buf, DirInode* inode)
{
   // note: the total amount of serialized data may not be larger than META_SERBUF_SIZE

   inode->featureFlags |= (DIRINODE_FEATURE_EARLY_SUBDIRS | DIRINODE_FEATURE_STATFLAGS);

   size_t bufPos = 0;

   // DiskMetaDataType (1 byte)
   bufPos += Serialization::serializeUInt8(&buf[bufPos], DiskMetaDataType_DIRINODE);

   // metadata format version (1 byte); note: we have only one format currently
   bufPos += Serialization::serializeUInt8(&buf[bufPos], (uint8_t) DIRECTORY_STORAGE_FORMAT_VER);

   // FeatureFlags (2 byte)
   bufPos += Serialization::serializeUInt16(&buf[bufPos], inode->featureFlags);

   // numSubdirs (4 byte), moved here with feature flag for 8 byte statData alignment
   bufPos += Serialization::serializeUInt(&buf[bufPos], inode->numSubdirs);

   // statData (starts with 8 byte, so we need to be 8 byte aligned here, ends with 4 bytes)
   bufPos += inode->statData.serialize(true, true, false, &buf[bufPos]);

   // numFiles
   bufPos += Serialization::serializeUInt(&buf[bufPos], inode->numFiles);

   // ID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], inode->id.size(), inode->id.c_str() );

   // parentDirID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], inode->parentDirID.size(),
      inode->parentDirID.c_str() );

   // ownerNodeID
   bufPos += Serialization::serializeUShort(&buf[bufPos], inode->ownerNodeID);

   // parentNodeID
   bufPos += Serialization::serializeUShort(&buf[bufPos], inode->parentNodeID);

   // mirrorNodeID (depends on feature flag)
   if(inode->featureFlags & DIRINODE_FEATURE_MIRRORED)
      bufPos += Serialization::serializeUInt16(&buf[bufPos], inode->mirrorNodeID);

   // stripePattern
   bufPos += inode->stripePattern->serialize(&buf[bufPos]);

#ifdef BEEGFS_HSM_DEPRECATED
   // HsmFileMetaData
   if (inode->featureFlags & DIRINODE_FEATURE_HSM)
      bufPos += Serialization::serializeUShort(&buf[bufPos], inode->hsmCollocationID);
#endif

   return bufPos;
}

/*
 * Deserialize a DirInode
 *
 * Note: Applies deserialized data directly to the current object
 *
 */
bool DiskMetaData::deserializeDirInode(const char* buf, DirInode* outInode)
{
   const char* logContext = DISKMETADATA_LOG_CONTEXT " (DirInode Deserialization)";
   // note: assumes that the total amount of serialized data may not be larger than META_SERBUF_SIZE

   size_t bufLen = META_SERBUF_SIZE;
   size_t bufPos = 0;

   { // DiskMetaDataType
      uint8_t metaDataType;
      unsigned metaDataTypeLen;

      if(!Serialization::deserializeUInt8(&buf[bufPos], bufLen-bufPos, &metaDataType,
         &metaDataTypeLen) )
      {
         std::string serialType = "DiskMetaDataType";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      if (unlikely(metaDataType != DiskMetaDataType_DIRINODE))
      {
         LogContext(logContext).logErr(
            std::string("Deserialization failed: expected DirInode, but got (numeric type): ")
            + StringTk::uintToStr((unsigned) metaDataType) );

         return false;
      }

      bufPos += metaDataTypeLen;
   }

   { // metadata format version
      unsigned formatVersionFieldLen;
      uint8_t formatVersionBuf; // we have only one format currently

      if(!Serialization::deserializeUInt8(&buf[bufPos], bufLen-bufPos, &formatVersionBuf,
         &formatVersionFieldLen) )
      {
         std::string serialType = "format version";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bufPos += formatVersionFieldLen;
   }

   { // feature flags
      unsigned featureFlagsLen;

      if ( !Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos,
         &(outInode->featureFlags), &featureFlagsLen) )
      {
         std::string serialType = "feature flags";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bool compatCheckRes = checkFeatureFlagsCompat(
         outInode->featureFlags, getSupportedDirInodeFeatureFlags() );
      if(unlikely(!compatCheckRes) )
      {
         LogContext(logContext).logErr("Incompatible DirInode feature flags found. "
            "Used flags (hex): " + StringTk::uintToHexStr(outInode->featureFlags) + "; "
            "Supported (hex): " + StringTk::uintToHexStr(getSupportedDirInodeFeatureFlags() ) );

         return false;
      }

      bufPos += featureFlagsLen;
   }

   if (likely (outInode->featureFlags & DIRINODE_FEATURE_EARLY_SUBDIRS) )
   { // numSubdirs
      unsigned subdirsFieldLen;

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &(outInode->numSubdirs),
         &subdirsFieldLen) )
      {
         std::string serialType = "numSubDirs";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bufPos += subdirsFieldLen;
   }


   { // statData
      bool hasStatFlags = (outInode->featureFlags & DIRINODE_FEATURE_STATFLAGS) ? true : false;
      unsigned statFieldLen;

      if (!outInode->statData.deserialize(true, hasStatFlags, false, &buf[bufPos], bufLen-bufPos,
         &statFieldLen) )
      {
         std::string serialType = "statData";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bufPos += statFieldLen;
   }

   if (unlikely(!(outInode->featureFlags & DIRINODE_FEATURE_EARLY_SUBDIRS) ) )
   { // numSubdirs
      unsigned subdirsFieldLen;

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &(outInode->numSubdirs),
         &subdirsFieldLen) )
      {
         std::string serialType = "numSubDirs";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bufPos += subdirsFieldLen;
   }

   { // numFiles
      unsigned filesFieldLen;

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &(outInode->numFiles),
         &filesFieldLen) )
      {
         std::string serialType = "numFiles";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bufPos += filesFieldLen;
   }

   { // ID
      unsigned idBufLen;
      unsigned idLen;
      const char* idStrStart;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &idLen, &idStrStart, &idBufLen) )
      {
         std::string serialType = "entryID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      outInode->id.assign(idStrStart, idLen);
      bufPos += idBufLen;
   }

   { // parentDirID
      unsigned parentDirBufLen;
      unsigned parentDirLen;
      const char* parentDirStrStart;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &parentDirLen, &parentDirStrStart, &parentDirBufLen) )
      {
         std::string serialType = "parentDirID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      outInode->parentDirID.assign(parentDirStrStart, parentDirLen);
      bufPos += parentDirBufLen;
   }

   { // ownerNodeID
      unsigned ownerBufLen;

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &(outInode->ownerNodeID), &ownerBufLen) )
      {
         std::string serialType = "ownerNodeID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bufPos += ownerBufLen;
   }

   { // parentNodeID
      unsigned parentNodeBufLen;

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &(outInode->parentNodeID), &parentNodeBufLen) )
      {
         std::string serialType = "parentNodeID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bufPos += parentNodeBufLen;
   }

   // mirrorNodeID (depends on feature flag)
   if(outInode->featureFlags & DIRINODE_FEATURE_MIRRORED)
   {
      unsigned mirrorNodeBufLen;

      if(!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
         &(outInode->mirrorNodeID), &mirrorNodeBufLen) )
      {
         std::string serialType = "mirrorNodeID";
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      bufPos += mirrorNodeBufLen;
   }

   { // stripePattern
      unsigned patternBufLen;
      StripePatternHeader patternHeader;
      const char* patternStart;
      std::string serialType = "stripePattern";

      if(!StripePattern::deserializePatternPreprocess(&buf[bufPos], bufLen-bufPos,
         &patternHeader, &patternStart, &patternBufLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);

         return false;
      }

      outInode->stripePattern = StripePattern::createFromBuf(patternStart, patternHeader);
      if (unlikely(outInode->stripePattern->getPatternType() == STRIPEPATTERN_Invalid) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         SAFE_DELETE(outInode->stripePattern);

         return false;
      }

      bufPos += patternBufLen;
   }

#ifdef BEEGFS_HSM_DEPRECATED
   {
      // hsmFileMetaData
      if (outInode->featureFlags & DIRINODE_FEATURE_HSM)
      {
         unsigned hsmCollocationIDBufLen;

         if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
            &(outInode->hsmCollocationID), &hsmCollocationIDBufLen) )
            return false;

         bufPos += hsmCollocationIDBufLen;
      }
   }
#endif


   return true;
}

/**
 * @return mask of supported dentry feature flags
 */
unsigned DiskMetaData::getSupportedDentryFeatureFlags()
{
   return DENTRY_FEATURE_INODE_INLINE | DENTRY_FEATURE_IS_FILEINODE | DENTRY_FEATURE_MIRRORED;
}


/**
 * @return mask of supported file inode feature flags, inlined into V4 Dentries
 */
unsigned DiskMetaData::getSupportedDentryV4FileInodeFeatureFlags()
{
   unsigned supportedFlags = FILEINODE_FEATURE_MIRRORED;

#ifdef BEEGFS_HSM_DEPRECATED
   supportedFlags |= FILEINODE_FEATURE_HSM;
#endif

   return supportedFlags;
}

/**
 * @return mask of supported file inode feature flags, inlined into V5 Dentries
 */
unsigned DiskMetaData::getSupportedDentryV5FileInodeFeatureFlags()
{
   unsigned supportedFlags = FILEINODE_FEATURE_MIRRORED |
      FILEINODE_FEATURE_HAS_ORIG_PARENTID | FILEINODE_FEATURE_HAS_ORIG_PARENTID |
      FILEINODE_FEATURE_HAS_ORIG_UID;

#ifdef BEEGFS_HSM_DEPRECATED
   supportedFlags |= FILEINODE_FEATURE_HSM;
#endif

   return supportedFlags;
}

/**
 * @return mask of supported dir inode feature flags
 */
unsigned DiskMetaData::getSupportedDirInodeFeatureFlags()
{
   unsigned supportedFlags =
      DIRINODE_FEATURE_EARLY_SUBDIRS | DIRINODE_FEATURE_MIRRORED | DIRINODE_FEATURE_STATFLAGS;

#ifdef BEEGFS_HSM_DEPRECATED
   supportedFlags |= DIRINODE_FEATURE_HSM;
#endif

   return supportedFlags;
}

/**
 * Compare usedFeatureFlags with supportedFeatureFlags to find out whether an unsupported
 * feature flag is used.
 *
 * @return false if an unsupported feature flag was set in usedFeatureFlags
 */
bool DiskMetaData::checkFeatureFlagsCompat(unsigned usedFeatureFlags,
   unsigned supportedFeatureFlags)
{
   unsigned unsupportedFlags = ~supportedFeatureFlags;

   if(unlikely(usedFeatureFlags & unsupportedFlags) )
      return false; // an unsupported flag was set

   return true;
}
