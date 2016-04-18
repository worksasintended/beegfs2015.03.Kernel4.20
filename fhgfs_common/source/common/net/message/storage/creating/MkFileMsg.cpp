#include "MkFileMsg.h"


bool MkFileMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   {// userID
      unsigned userIDLen;
      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &userID, &userIDLen) )
         return false;

      bufPos += userIDLen;
   }
   
   { // groupID
      unsigned groupIDLen;
      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &groupID, &groupIDLen) )
         return false;

      bufPos += groupIDLen;
   }

   { // mode
      unsigned modeLen;
      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &mode, &modeLen) )
         return false;

      bufPos += modeLen;
   }

   if (isMsgHeaderFeatureFlagSet(MKFILEMSG_FLAG_UMASK) )
   { // umask
      unsigned umaskLen;
      if (!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &umask, &umaskLen) )
         return false;

      bufPos += umaskLen;
   }

   // optional stripe hints
   if(isMsgHeaderFeatureFlagSet(MKFILEMSG_FLAG_STRIPEHINTS) )
   {
      { // numtargets
         unsigned numtargetsLen;
         if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
            &numtargets, &numtargetsLen) )
            return false;

         bufPos += numtargetsLen;
      }

      { // chunksize
         unsigned chunksizeLen;
         if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
            &chunksize, &chunksizeLen) )
            return false;

         bufPos += chunksizeLen;
      }
   }

   { // parentInfo
      unsigned parentLen;

      if(!this->parentInfo.deserialize(&buf[bufPos], bufLen-bufPos, &parentLen) )
         return false;

      bufPos += parentLen;
   }

   { // newName
      unsigned nameLen;

      if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &this->newNameLen, &this->newName, &nameLen) )
         return false;

      bufPos += nameLen;
   }

   { // preferredTargets
      if(!Serialization::deserializeUInt16ListPreprocess(&buf[bufPos], bufLen-bufPos,
         &prefTargetsElemNum, &prefTargetsListStart, &prefTargetsBufLen) )
         return false;

      bufPos += prefTargetsBufLen;
   }

   return true;
}

void MkFileMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // userID
   bufPos += Serialization::serializeUInt(&buf[bufPos], userID);
   
   // groupID
   bufPos += Serialization::serializeUInt(&buf[bufPos], groupID);

   // mode
   bufPos += Serialization::serializeInt(&buf[bufPos], mode);

   // optional umask
   if (isMsgHeaderFeatureFlagSet(MKFILEMSG_FLAG_UMASK) )
      bufPos += Serialization::serializeInt(&buf[bufPos], umask);

   // optional stripe hints
   if(isMsgHeaderFeatureFlagSet(MKFILEMSG_FLAG_STRIPEHINTS) )
   {
      // userID
      bufPos += Serialization::serializeUInt(&buf[bufPos], numtargets);

      // groupID
      bufPos += Serialization::serializeUInt(&buf[bufPos], chunksize);
   }

   // parentInfo
   bufPos += this->parentInfoPtr->serialize(&buf[bufPos]);

   // newName
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], this->newNameLen, this->newName);

   // preferredTargets
   bufPos += Serialization::serializeUInt16List(&buf[bufPos], preferredTargets);
}

