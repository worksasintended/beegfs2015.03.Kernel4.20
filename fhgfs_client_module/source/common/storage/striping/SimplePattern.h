#ifndef SIMPLEPATTERN_H_
#define SIMPLEPATTERN_H_

#include "StripePattern.h"

struct SimplePattern;
typedef struct SimplePattern SimplePattern;


static inline void SimplePattern_init(SimplePattern* this,
   unsigned patternType, unsigned chunkSize);
static inline SimplePattern* SimplePattern_construct(
   unsigned patternType, unsigned chunkSize);
static inline void SimplePattern_uninit(StripePattern* this);
static inline void SimplePattern_destruct(StripePattern* this);

// virtual functions
extern void SimplePattern_serializePattern(StripePattern* this, char* buf);
extern fhgfs_bool SimplePattern_deserializePattern(StripePattern* this,
   const char* buf, size_t bufLen);
extern unsigned SimplePattern_serialLen(StripePattern* this);

extern size_t SimplePattern_getStripeTargetIndex(StripePattern* this, int64_t pos);
extern uint16_t SimplePattern_getStripeTargetID(StripePattern* this, int64_t pos);
extern unsigned SimplePattern_getMinNumTargets(StripePattern* this);
extern unsigned SimplePattern_getDefaultNumTargets(StripePattern* this);
extern void SimplePattern_getStripeTargetIDsCopy(StripePattern* this, UInt16Vec* outTargetIDs);
extern UInt16Vec* SimplePattern_getStripeTargetIDs(StripePattern* this);



struct SimplePattern
{
   StripePattern stripePattern;
};


void SimplePattern_init(SimplePattern* this,
   unsigned patternType, unsigned chunkSize)
{
   StripePattern_initFromPatternType( (StripePattern*)this, patternType, chunkSize);
   
   // assign virtual functions
   ( (StripePattern*)this)->uninit = SimplePattern_uninit;

   ( (StripePattern*)this)->serializePattern = SimplePattern_serializePattern;
   ( (StripePattern*)this)->deserializePattern = SimplePattern_deserializePattern;
   ( (StripePattern*)this)->serialLen = SimplePattern_serialLen;
   
   ( (StripePattern*)this)->getStripeTargetIndex = SimplePattern_getStripeTargetIndex;
   ( (StripePattern*)this)->getStripeTargetID = SimplePattern_getStripeTargetID;
   ( (StripePattern*)this)->getStripeTargetIDsCopy = SimplePattern_getStripeTargetIDsCopy;
   ( (StripePattern*)this)->getStripeTargetIDs = SimplePattern_getStripeTargetIDs;
   ( (StripePattern*)this)->getMinNumTargets = SimplePattern_getMinNumTargets;
   ( (StripePattern*)this)->getDefaultNumTargets = SimplePattern_getDefaultNumTargets;
}

SimplePattern* SimplePattern_construct(unsigned patternType, unsigned chunkSize)
{
   struct SimplePattern* this = os_kmalloc(sizeof(struct SimplePattern) );
   
   SimplePattern_init(this, patternType, chunkSize);
   
   return this;
}

void SimplePattern_uninit(StripePattern* this)
{
   StripePattern_uninit( (StripePattern*)this);
}

void SimplePattern_destruct(StripePattern* this)
{
   SimplePattern_uninit( (StripePattern*)this);
   
   os_kfree(this);
}

#endif /*SIMPLEPATTERN_H_*/
