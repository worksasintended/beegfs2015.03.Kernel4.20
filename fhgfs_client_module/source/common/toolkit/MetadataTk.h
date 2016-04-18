#ifndef METADATATK_H_
#define METADATATK_H_

#include <app/App.h>
#include <common/Common.h>
#include <common/nodes/Node.h>
#include <common/storage/Path.h>
#include <common/storage/StorageErrors.h>
#include <common/storage/EntryInfo.h>
#include <common/storage/StorageErrors.h>
#include <common/storage/StorageDefinitions.h>
#include <common/toolkit/list/UInt16List.h>
#include <common/toolkit/MetadataTk.h>
#include <os/OsTypeConversion.h>

#include "LookupIntentInfoOut.h"

#include <linux/fs.h>

#define METADATATK_OWNERSEARCH_MAX_STEPS    128 /* to avoid infinite searching */


struct CreateInfo;
typedef struct CreateInfo CreateInfo;

struct OpenInfo;
typedef struct OpenInfo OpenInfo;

struct LookupIntentInfoIn;
typedef struct LookupIntentInfoIn LookupIntentInfoIn;


extern fhgfs_bool MetadataTk_getRootEntryInfoCopy(App* app, EntryInfo* outEntryInfo);

// inliners

void CreateInfo_init(App* app, struct inode* parentDirInode, const char* entryName,
   int mode, fhgfs_bool isExclusiveCreate, struct CreateInfo* outCreateInfo);
static inline void LookupIntentInfoIn_init(LookupIntentInfoIn* this,
   const EntryInfo* parentEntryInfo, const char* entryName);
static inline void LookupIntentInfoIn_addEntryInfo(LookupIntentInfoIn* this,
   const EntryInfo* entryInfo);
static inline void LookupIntentInfoIn_addOpenInfo(LookupIntentInfoIn* this,
   const OpenInfo* openInfo);
static inline void LookupIntentInfoIn_addCreateInfo(LookupIntentInfoIn* this,
   CreateInfo* createInfo);
static inline void OpenInfo_init(OpenInfo* this, int accessFlags, fhgfs_bool isPagedMode);
static inline void LookupIntentInfoOut_prepare(LookupIntentInfoOut* this,
   EntryInfo* outEntryInfo, fhgfs_stat* outFhgfsStat);



/**
 * File/Dir create information. Should be initialized using CreateInfo_init().
 */
struct CreateInfo
{
      const char* entryName; // file name
      unsigned userID;
      unsigned groupID;
      int mode;
      int umask;
      UInt16List* preferredMetaTargets;
      UInt16List* preferredStorageTargets;
      fhgfs_bool isExclusiveCreate; // only for open() and creat(), is O_CREAT set?
};

struct OpenInfo
{
      int accessFlags; // fhgfs flags, not in-kernel open/access flags
      // const char* sessionID; // set at lower level on initializing LookupIntentMsg
};

/**
 * In-Args for _lookupCreateStat operations. Init only with LookupIntentInfoIn_init().
 */
struct LookupIntentInfoIn
{
   const EntryInfo* parentEntryInfo;
   const EntryInfo* entryInfoPtr;      // only set on revalidate
   const char* entryName;              // file name

   CreateInfo* createInfo;       // only set on file create
   fhgfs_bool isExclusiveCreate;       // fhgfs_true iff O_EXCL is set

   const OpenInfo* openInfo;           // only set on file open
};


/**
 * @param accessFlags is the same as openFlags
 */
void OpenInfo_init(OpenInfo* this, int accessFlags, fhgfs_bool isPagedMode)
{
   this->accessFlags = OsTypeConv_openFlagsOsToFhgfs(accessFlags, isPagedMode);

   // this->sessionID = sessionID; // see OpenInfo definition
}

static inline void LookupIntentInfoIn_init(LookupIntentInfoIn* this,
   const EntryInfo* parentEntryInfo, const char* entryName)
{
   this->parentEntryInfo   = parentEntryInfo;
   this->entryName         = entryName;
   this->entryInfoPtr      = NULL;
   this->openInfo          = NULL;
   this->createInfo        = NULL;
}

void LookupIntentInfoIn_addEntryInfo(LookupIntentInfoIn* this, const EntryInfo* entryInfo)
{
   this->entryInfoPtr = entryInfo;
}


void LookupIntentInfoIn_addOpenInfo(LookupIntentInfoIn* this, const OpenInfo* openInfo)
{
   this->openInfo = openInfo;
}

void LookupIntentInfoIn_addCreateInfo(LookupIntentInfoIn* this, CreateInfo* createInfo)
{
   this->createInfo        = createInfo;

   if (likely(createInfo) )
      this->isExclusiveCreate = createInfo->isExclusiveCreate;
}

#endif /*METADATATK_H_*/
