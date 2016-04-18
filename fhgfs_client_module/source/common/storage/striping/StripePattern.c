#include <common/toolkit/Serialization.h>
#include "BuddyMirrorPattern.h"
#include "Raid0Pattern.h"
#include "Raid10Pattern.h"
#include "SimplePattern.h"
#include "StripePattern.h"


/**
 * Calls the virtual uninit method and kfrees the object.
 */
void StripePattern_virtualDestruct(StripePattern* this)
{
   this->uninit(this);
   os_kfree(this);
}

/**
 * @return outPattern; outPattern->patternType is STRIPEPATTERN_Invalid on error
 */
StripePattern* StripePattern_createFromBuf(const char* patternStart,
   struct StripePatternHeader* patternHeader)
{
   StripePattern* pattern;
   const char* patternSpecificBuf;
   unsigned patternSpecificBufLen;
   fhgfs_bool deserRes;

   switch(patternHeader->patternType)
   {
      case STRIPEPATTERN_Raid0:
      {
         pattern = (StripePattern*)Raid0Pattern_constructFromChunkSize(patternHeader->chunkSize);
      } break;

      case STRIPEPATTERN_Raid10:
      {
         pattern = (StripePattern*)Raid10Pattern_constructFromChunkSize(patternHeader->chunkSize);
      } break;

      case STRIPEPATTERN_BuddyMirror:
      {
         pattern = (StripePattern*)BuddyMirrorPattern_constructFromChunkSize(
            patternHeader->chunkSize);
      } break;

      default:
      {
         pattern = (StripePattern*)SimplePattern_construct(STRIPEPATTERN_Invalid, 0);
         return pattern;
      } break;
   }

   patternSpecificBuf = patternStart + STRIPEPATTERN_HEADER_LENGTH;
   patternSpecificBufLen = patternHeader->patternLength - STRIPEPATTERN_HEADER_LENGTH;

   deserRes = pattern->deserializePattern(pattern, patternSpecificBuf, patternSpecificBufLen);
   if(unlikely(!deserRes) )
   { // deserialization failed => discard half-initialized pattern and create new invalid pattern
      StripePattern_virtualDestruct(pattern);

      pattern = (StripePattern*)SimplePattern_construct(STRIPEPATTERN_Invalid, 0);

      return pattern;
   }

   return pattern;

}


void __StripePattern_serializeHeader(StripePattern* this, char* buf)
{
   size_t bufPos = 0;

   // pattern length

   bufPos += Serialization_serializeUInt(&buf[bufPos], StripePattern_getSerialPatternLength(this) );

   // pattern type

   bufPos += Serialization_serializeUInt(&buf[bufPos], this->patternType);

   // chunkSize

   bufPos += Serialization_serializeUInt(&buf[bufPos], this->chunkSize);

}

fhgfs_bool __StripePattern_deserializeHeader(const char* buf, size_t bufLen,
   struct StripePatternHeader* outPatternHeader)
{
   size_t bufPos = 0;

   unsigned lenInfoFieldLen;
   unsigned typeInfoFieldLen;
   unsigned chunkSizeInfoFieldLen;

   // pattern length
   if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos,
      &outPatternHeader->patternLength, &lenInfoFieldLen) )
      return fhgfs_false;

   bufPos += lenInfoFieldLen;

   // pattern type
   if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos,
      &outPatternHeader->patternType, &typeInfoFieldLen) )
      return fhgfs_false;

   bufPos += typeInfoFieldLen;

   // chunkSize
   if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos,
      &outPatternHeader->chunkSize, &chunkSizeInfoFieldLen) )
      return fhgfs_false;

   bufPos += chunkSizeInfoFieldLen;


   // check length field
   if(outPatternHeader->patternLength < STRIPEPATTERN_HEADER_LENGTH)
      return fhgfs_false;

   // check chunkSize
   if(!outPatternHeader->chunkSize)
      return fhgfs_false;


   return fhgfs_true;
}

/**
 * Predefined virtual method returning NULL. Will be overridden by StripePatterns (e.g. Raid10)
 * that actually do have mirror targets.
 *
 * @return NULL for patterns that don't have mirror targets.
 */
UInt16Vec* StripePattern_getMirrorTargetIDs(StripePattern* this)
{
   return NULL;
}

/**
 * Returns human-readable pattern type.
 *
 * @return static string (not alloced => don't free it)
 */
const char* StripePattern_getPatternTypeStr(StripePattern* this)
{
   switch(this->patternType)
   {
      case STRIPEPATTERN_Invalid:
      {
         return "<invalid>";
      } break;

      case STRIPEPATTERN_Raid0:
      {
         return "RAID0";
      } break;

      case STRIPEPATTERN_Raid10:
      {
         return "RAID10";
      } break;

      default:
      {
         return "<unknown>";
      } break;
   }
}
