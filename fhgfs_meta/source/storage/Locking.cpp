#include <common/toolkit/serialization/Serialization.h>
#include <common/toolkit/Random.h>
#include "Locking.h"



unsigned EntryLockDetails::serialize(char* buf) const
{
   size_t bufPos = 0;

   // clientID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], clientID.size(), clientID.c_str() );

   // clientFD
   bufPos += Serialization::serializeInt64(&buf[bufPos], clientFD);

   // ownerPID
   bufPos += Serialization::serializeInt(&buf[bufPos], ownerPID);

   // lockAckID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], lockAckID.size(), lockAckID.c_str() );

   // lockTypeFlags
   bufPos += Serialization::serializeInt(&buf[bufPos], lockTypeFlags);

   return bufPos;
}

bool EntryLockDetails::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   size_t bufPos = 0;

   {
      // clientID
      unsigned clientIDLen;

      if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos, &clientID,
         &clientIDLen) )
         return false;

      bufPos += clientIDLen;
   }

   {
      // clientFD
      unsigned clientFDLen;

      if (!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &clientFD, &clientFDLen) )
         return false;

      bufPos += clientFDLen;
   }

   {
      // ownerPID
      unsigned ownerPIDLen;

      if (!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &ownerPID, &ownerPIDLen) )
         return false;

      bufPos += ownerPIDLen;
   }

   {
      // lockAckID
      unsigned lockAckIDLen;

      if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos, &lockAckID,
         &lockAckIDLen) )
         return false;

      bufPos += lockAckIDLen;
   }

   {
      // lockTypeFlags
      unsigned lockTypeFlagsLen;

      if (!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &lockTypeFlags,
         &lockTypeFlagsLen) )
         return false;

      bufPos += lockTypeFlagsLen;
   }

   *outLen = bufPos;

   return true;
}

unsigned EntryLockDetails::serialLen() const
{
   unsigned len = 0;

   len += Serialization::serialLenStrAlign4(clientID.size() );    // clientID
   len += Serialization::serialLenInt64();                        // clientFD
   len += Serialization::serialLenInt();                          // ownerPID
   len += Serialization::serialLenStrAlign4(lockAckID.size() );   // lockAckID
   len += Serialization::serialLenInt();                          // lockTypeFlags

   return len;
}

unsigned RangeLockDetails::serialize(char* buf) const
{
   size_t bufPos = 0;

   // clientID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], clientID.size(), clientID.c_str() );

   // ownerPID
   bufPos += Serialization::serializeInt(&buf[bufPos], ownerPID);

   // lockAckID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], lockAckID.size(), lockAckID.c_str() );

   // lockTypeFlags
   bufPos += Serialization::serializeInt(&buf[bufPos], lockTypeFlags);

   // clientFD
   bufPos += Serialization::serializeUInt64(&buf[bufPos], start);

   // clientFD
   bufPos += Serialization::serializeUInt64(&buf[bufPos], end);

   return bufPos;
}

bool RangeLockDetails::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   size_t bufPos = 0;

   {
      // clientID
      unsigned clientIDLen;

      if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos, &clientID,
         &clientIDLen) )
         return false;

      bufPos += clientIDLen;
   }

   {
      // ownerPID
      unsigned ownerPIDLen;

      if (!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &ownerPID, &ownerPIDLen) )
         return false;

      bufPos += ownerPIDLen;
   }

   {
      // lockAckID
      unsigned lockAckIDLen;

      if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos, &lockAckID,
         &lockAckIDLen) )
         return false;

      bufPos += lockAckIDLen;
   }

   {
      // lockTypeFlags
      unsigned lockTypeFlagsLen;

      if (!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &lockTypeFlags,
         &lockTypeFlagsLen) )
         return false;

      bufPos += lockTypeFlagsLen;
   }

   {
      // start
      unsigned startLen;

      if (!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos, &start, &startLen) )
         return false;

      bufPos += startLen;
   }

   {
      // end
      unsigned endLen;

      if (!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos, &end, &endLen) )
         return false;

      bufPos += endLen;
   }

   *outLen = bufPos;

   return true;
}

unsigned RangeLockDetails::serialLen() const
{
   unsigned len = 0;

   len += Serialization::serialLenStrAlign4(clientID.size() );    // clientID
   len += Serialization::serialLenInt();                          // ownerPID
   len += Serialization::serialLenStrAlign4(lockAckID.size() );   // lockAckID
   len += Serialization::serialLenInt();                          // lockTypeFlags
   len += Serialization::serialLenUInt64();                       // start
   len += Serialization::serialLenUInt64();                       // end

   return len;
}



bool entryLockDetailsEquals(const EntryLockDetails& first, const EntryLockDetails& second)
{
   // clientID
   if(first.clientID != second.clientID)
      return false;

   // clientFD
   if(first.clientFD != second.clientFD)
      return false;

   // ownerPID
   if(first.ownerPID != second.ownerPID)
      return false;

   // lockAckID
   if(first.lockAckID != second.lockAckID)
      return false;

   // lockTypeFlags
   if(first.lockTypeFlags != second.lockTypeFlags)
      return false;

   return true;
}

bool rangeLockDetailsEquals(const RangeLockDetails& first, const RangeLockDetails& second)
{
   // clientID
   if(first.clientID != second.clientID)
      return false;

   // ownerPID
   if(first.ownerPID != second.ownerPID)
      return false;

   // lockAckID
   if(first.lockAckID != second.lockAckID)
      return false;

   // lockTypeFlags
   if(first.lockTypeFlags != second.lockTypeFlags)
      return false;

   // start
   if(first.start != second.start)
      return false;

   // end
   if(first.end != second.end)
      return false;

   return true;
}



void EntryLockDetails::initRandomForSerializationTests()
{
   Random rand;

   this->clientFD = rand.getNextInt();
   StringTk::genRandomAlphaNumericString(this->clientID, rand.getNextInRange(0, 30) );
   StringTk::genRandomAlphaNumericString(this->lockAckID, rand.getNextInRange(0, 30) );
   this->lockTypeFlags = rand.getNextInt();
   this->ownerPID = rand.getNextInt();
}

void RangeLockDetails::initRandomForSerializationTests()
{
   Random rand;

   StringTk::genRandomAlphaNumericString(this->clientID, rand.getNextInRange(0, 30) );
   this->end = rand.getNextInt();
   StringTk::genRandomAlphaNumericString(this->lockAckID, rand.getNextInRange(0, 30) );
   this->lockTypeFlags = rand.getNextInt();
   this->ownerPID = rand.getNextInt();
   this->start = rand.getNextInt();
}
