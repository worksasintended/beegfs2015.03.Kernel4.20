#include "FsckTk.h"

#include <common/app/log/LogContext.h>
#include <common/storage/striping/BuddyMirrorPattern.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/storage/striping/SimplePattern.h>
#include <common/toolkit/StringTk.h>

FsckDirEntryType FsckTk::DirEntryTypeToFsckDirEntryType(DirEntryType entryType)
{
   FsckDirEntryType retVal;

   if (DirEntryType_ISREGULARFILE(entryType))
      retVal = FsckDirEntryType_REGULARFILE;
   else
   if (DirEntryType_ISSYMLINK(entryType))
      retVal = FsckDirEntryType_SYMLINK;
   else
   if (DirEntryType_ISDIR(entryType))
      retVal = FsckDirEntryType_DIRECTORY;
   else
   if (! DirEntryType_ISVALID(entryType))
      retVal = FsckDirEntryType_INVALID;
   else // is valid, but must be something special (dev, fifo, socket, ...)
      retVal = FsckDirEntryType_SPECIAL;

   return retVal;
}

/*
 * @param StripePattern pointer to a fhgfs stripe pattern
 * @outTargetIDVector
 * @outChunkSize
 *
 * @return a the corresponding FsckStripePatternType
 */
FsckStripePatternType FsckTk::stripePatternToFsckStripePattern(StripePattern* stripePattern,
   unsigned *outChunkSize, UInt16Vector* outTargetIDVector)
{
   // convert the pattern type
   FsckStripePatternType patternType;

   switch (stripePattern->getPatternType())
   {
      case (STRIPEPATTERN_Raid0):
         patternType = FsckStripePatternType_RAID0;
         break;
      case (STRIPEPATTERN_BuddyMirror):
         patternType = FsckStripePatternType_BUDDYMIRROR;
         break;
      default:
         patternType = FsckStripePatternType_INVALID;
         break;
   }

   if ( stripePattern->getPatternType() != FsckStripePatternType_INVALID )
   {
      // get the targetIDs
      if ( outTargetIDVector )
         stripePattern->getStripeTargetIDs(outTargetIDVector);

      // get the chunkSize
      if ( outChunkSize )
         *outChunkSize = stripePattern->getChunkSize();
   }

   return patternType;
}

/*
 * @param targetVecBuf a buffer holding the serialized targetIDVector
 * @param stripePatternType the type of the pattern as FsckStripePatternType
 * @param chunkSize the chunk size to set in the pattern
 *
 * @return a pointer to a stripe pattern; make sure to delete after use
 */
StripePattern* FsckTk::FsckStripePatternToStripePattern(FsckStripePatternType stripePatternType,
   unsigned chunkSize, UInt16Vector* targetIDVector)
{
   StripePattern* pattern = NULL;

   switch(stripePatternType)
   {
      case (FsckStripePatternType_RAID0):
      {
         pattern = new Raid0Pattern(chunkSize, *targetIDVector);
         break;
      }

      case (FsckStripePatternType_BUDDYMIRROR):
      {
         pattern = new BuddyMirrorPattern(chunkSize, *targetIDVector);
         break;
      }

      case (FsckStripePatternType_INVALID):
      {
         pattern = new SimplePattern(STRIPEPATTERN_Invalid, 0);
         break;
      }
      default:
      {
         pattern = new SimplePattern(STRIPEPATTERN_Invalid, 0);
         break;
      }
   }

   return pattern;
}

