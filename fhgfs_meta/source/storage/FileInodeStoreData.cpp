#include "FileInodeStoreData.h"

void FileInodeStoreData::getPathInfo(PathInfo* outPathInfo)
{
   const char* logContext = "FileInode getPathInfo";
   unsigned flags;

   FileInodeOrigFeature origFeature  = getOrigFeature();
   switch (origFeature)
   {
      case FileInodeOrigFeature_TRUE:
      {
         flags = PATHINFO_FEATURE_ORIG;
      } break;

      case FileInodeOrigFeature_FALSE:
      {
         flags = 0;
      } break;

      default:
      case FileInodeOrigFeature_UNSET:
      {
         flags = PATHINFO_FEATURE_ORIG_UNKNOWN;
         LogContext(logContext).logErr("Bug: Unknown PathInfo status.");
      } break;

   }

   unsigned origParentUID = getOrigUID();
   std::string origParentEntryID = getOrigParentEntryID();

   outPathInfo->set(origParentUID, origParentEntryID, flags);
}

unsigned FileInodeStoreData::serialize(char* buf) const
{
   size_t bufPos = 0;

   // inodeFeatureFlags
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->inodeFeatureFlags);

   // inodeStatData
   bufPos += inodeStatData.serialize(false, true, false, &buf[bufPos]);

   // entryID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], this->entryID.size(),
      this->entryID.c_str() );

   // stripePattern
   bufPos += stripePattern->serialize(&buf[bufPos]);

   // origFeature
   bufPos += Serialization::serializeInt(&buf[bufPos], this->origFeature);

   // origParentUID
   bufPos += Serialization::serializeInt(&buf[bufPos], this->origParentUID);

   // origParentEntryID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], this->origParentEntryID.size(),
      this->origParentEntryID.c_str() );

   // hsmFileMetaData
   bufPos += hsmFileMetaData.serialize(&buf[bufPos]);

   return bufPos;
}

bool FileInodeStoreData::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   size_t bufPos = 0;

   {
      // inodeFeatureFlags
      unsigned inodeFeatureFlagsLen;

      if (!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &this->inodeFeatureFlags,
         &inodeFeatureFlagsLen) )
         return false;

      bufPos += inodeFeatureFlagsLen;
   }

   {
      // inodeStatData
      unsigned inodeStatDataLen;

      if (!this->inodeStatData.deserialize(false, true, false, &buf[bufPos], bufLen-bufPos,
         &inodeStatDataLen) )
         return false;

      bufPos += inodeStatDataLen;
   }

   {
      // entryID
      unsigned entryIDLen;

      if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos, &this->entryID,
         &entryIDLen) )
         return false;

      bufPos += entryIDLen;
   }

   {
      // stripePattern
      unsigned patternBufLen;

      StripePatternHeader patternHeader;
      const char* patternStart;

      if(!StripePattern::deserializePatternPreprocess(&buf[bufPos], bufLen-bufPos,
         &patternHeader, &patternStart, &patternBufLen) )
         return false;

      this->stripePattern = StripePattern::createFromBuf(patternStart, patternHeader);

      bufPos += patternBufLen;
   }

   {
      // origFeature
      unsigned origFeatureLen;

      if (!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, (int*) &this->origFeature,
         &origFeatureLen) )
         return false;

      bufPos += origFeatureLen;
   }

   {
      // origParentUID
      unsigned origParentUIDLen;

      if (!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &this->origParentUID,
         &origParentUIDLen) )
         return false;

      bufPos += origParentUIDLen;
   }

   {
      // origParentEntryID
      unsigned origParentEntryIDLen;

      if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &this->origParentEntryID, &origParentEntryIDLen) )
         return false;

      bufPos += origParentEntryIDLen;
   }

   {
      // hsmFileMetaData
      unsigned hsmFileMetaDataLen;

      if (!this->hsmFileMetaData.deserialize(&buf[bufPos], bufLen-bufPos, &hsmFileMetaDataLen) )
         return false;

      bufPos += hsmFileMetaDataLen;
   }

   *outLen = bufPos;

   return true;
}

unsigned FileInodeStoreData::serialLen() const
{
   unsigned len = 0;

   len += Serialization::serialLenUInt();                                     // inodeFeatureFlags
   len += inodeStatData.serialLen(false, true, false);                        // inodeStatData
   len += Serialization::serialLenStrAlign4(entryID.length() );               // entryID
   len += stripePattern->getSerialPatternLength();                            // stripePattern
   len += Serialization::serialLenInt();                                      // origFeature
   len += Serialization::serialLenUInt();                                     // origParentUID
   len += Serialization::serialLenStrAlign4(origParentEntryID.length() );     // origParentEntryID
   len += hsmFileMetaData.serialLen();                                        // hsmFileMetaData

   return len;
}

bool fileInodeStoreDataEquals(const FileInodeStoreData& first, const FileInodeStoreData& second)
{
   // inodeFeatureFlags
   if(first.inodeFeatureFlags != second.inodeFeatureFlags)
      return false;

   // inodeStatData
   if(!statDataEquals(first.inodeStatData, second.inodeStatData) )
      return false;

   // entryID
   if(first.entryID != second.entryID)
      return false;

   // stripePattern
   if(!first.stripePattern->stripePatternEquals(second.stripePattern) )
      return false;

   // origFeature
   if(first.origFeature != second.origFeature)
      return false;

   // origParentUID
   if(first.origParentUID != second.origParentUID)
      return false;

   // origParentEntryID
   if(first.origParentEntryID != second.origParentEntryID)
      return false;

   // hsmFileMetaData
   if(!hsmFileMetaDataEquals(first.hsmFileMetaData, second.hsmFileMetaData) )
      return false;

   return true;
}
