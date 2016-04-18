#include "HardlinkMsg.h"


bool HardlinkMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   { // fromInfo
      unsigned fromBufLen;

      if(!this->fromInfo.deserialize(&buf[bufPos], bufLen-bufPos, &fromBufLen) )
         return false;

      bufPos += fromBufLen;
   }

   { // toDirInfo
      unsigned toBufLen;

      if (!this->toDirInfo.deserialize(&buf[bufPos], bufLen-bufPos, &toBufLen) )
         return false;

      bufPos += toBufLen;
   }

   { // toName

      unsigned newBufLen;
      const char* newNameChar;
      unsigned newNameLen;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &newNameLen, &newNameChar, &newBufLen) )
         return false;

      this->toName.assign(newNameChar, newNameLen);
      bufPos += newBufLen;
   }

   int flags = this->getMsgHeaderFeatureFlags();

   if ((flags & HARDLINK_FLAG_IS_TO_DENTRY_CREATE) )
   {
      // remote dir entry create, no 'from' data not included into NetMsg
   }
   else
   {  // NetMsg received from a client, including 'from' data

      { // fromDirInfo
         unsigned fromBufLen;

         if(!this->fromDirInfo.deserialize(&buf[bufPos], bufLen-bufPos, &fromBufLen) )
            return false;

         bufPos += fromBufLen;
      }

      { // fromName

         unsigned oldBufLen;
         const char* oldNameChar;
         unsigned oldNameLen;

         if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
            &oldNameLen, &oldNameChar, &oldBufLen) )
            return false;

         this->fromName.assign(oldNameChar, oldNameLen);
         bufPos += oldBufLen;
      }
   }

   return true;
}

void HardlinkMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // fromInfo
   bufPos += this->fromInfoPtr->serialize(&buf[bufPos]);

   // toDirInfo
   bufPos += this->toDirInfoPtr->serialize(&buf[bufPos]);

   // toName
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
      this->toName.length(), this->toName.c_str() );

   if (this->fromDirInfoPtr)
   {  // message is being send from a client
      // fromDirInfo
      bufPos += this->fromDirInfoPtr->serialize(&buf[bufPos]);

      // fromName
      bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
         this->fromName.length(), this->fromName.c_str() );

   }
   else
   {
      /* Message is being send from one meta server to the other to create the remote dentry,
       * not all information from the client included */
   }

}

/**
 * Compare cloneMsg (serialized, then deserialized) with our values
 */
TestingEqualsRes HardlinkMsg::testingEquals(NetMessage* cloneMsg)
{
   HardlinkMsg* cloneLinkMsg = (HardlinkMsg*) cloneMsg;

   if (!this->fromInfoPtr->compare(cloneLinkMsg->getFromInfo() ) )
      return TestingEqualsRes_FALSE;

   if (!this->toDirInfoPtr->compare(cloneLinkMsg->getToDirInfo() ) )
      return TestingEqualsRes_FALSE;

   if (this->toName.compare(cloneLinkMsg->getToName() ) )
      return TestingEqualsRes_FALSE;

   if (!(this->getMsgHeaderFeatureFlags() & HARDLINK_FLAG_IS_TO_DENTRY_CREATE) )
   {
      if (!this->fromDirInfoPtr->compare(cloneLinkMsg->getFromDirInfo() ) )
         return TestingEqualsRes_FALSE;

      if (this->fromName.compare(cloneLinkMsg->getFromName() ) )
         return TestingEqualsRes_FALSE;
   }

   return TestingEqualsRes_TRUE;
}
