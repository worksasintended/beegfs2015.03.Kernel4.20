#include "SimplePattern.h"

void SimplePattern_serializePattern(StripePattern* this, char* buf)
{
   // nothing to be done here
}

fhgfs_bool SimplePattern_deserializePattern(StripePattern* this, const char* buf, size_t bufLen)
{
   return fhgfs_true;
}

unsigned SimplePattern_serialLen(StripePattern* this)
{
   return STRIPEPATTERN_HEADER_LENGTH;
}

size_t SimplePattern_getStripeTargetIndex(StripePattern* this, int64_t pos)
{
   return 0;
}

uint16_t SimplePattern_getStripeTargetID(StripePattern* this, int64_t pos)
{
   return 0;
}

unsigned SimplePattern_getMinNumTargets(StripePattern* this)
{
   return 0;
}

unsigned SimplePattern_getDefaultNumTargets(StripePattern* this)
{
   return 0;
}

void SimplePattern_getStripeTargetIDsCopy(StripePattern* this, UInt16Vec* outTargetIDs)
{
   // nothing to be done here
}

UInt16Vec* SimplePattern_getStripeTargetIDs(StripePattern* this)
{
   return NULL;
}

