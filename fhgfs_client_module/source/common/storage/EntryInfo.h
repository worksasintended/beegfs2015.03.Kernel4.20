/*
 * class EntryInfo - required information to find an inode or chunk files
 *
 * NOTE: If you change this file, do not forget to adjust commons EntryInfo.h
 */

#ifndef ENTRYINFO_H_
#define ENTRYINFO_H_

#include <common/storage/StorageDefinitions.h>
#include <common/toolkit/StringTk.h>

#define ENTRYINFO_FEATURE_INLINED      1 // indicate inlined inode, might be outdated

#define EntryInfo_PARENT_ID_UNKNOWN    "unknown"

struct EntryInfo;
typedef struct EntryInfo EntryInfo;


static inline void EntryInfo_init(EntryInfo *this, uint16_t ownerNodeID,
   const char* parentEntryID, const char* entryID, const char* fileName, DirEntryType entryType,
   int flags);
static inline void EntryInfo_uninit(EntryInfo* this);

static inline EntryInfo* EntryInfo_construct(uint16_t ownerNodeID, const char* parentEntryID,
   const char* entryID, const char* fileName, DirEntryType entryType, int flags);
static inline void EntryInfo_destruct(EntryInfo* this);

static inline void EntryInfo_dup(const EntryInfo *inEntryInfo, EntryInfo *outEntryInfo);
static inline void EntryInfo_copy(const EntryInfo *inEntryInfo, EntryInfo *outEntryInfo);

static inline void EntryInfo_setOwnerNodeID(EntryInfo *this, uint16_t ownerNodeID);
static inline uint16_t EntryInfo_getOwnerNodeID(EntryInfo *this);
static inline void EntryInfo_setParentEntryID(EntryInfo *this, const char *parentEntryID);
static inline void EntryInfo_update(EntryInfo* this, const EntryInfo* newEntryInfo);
static inline void EntryInfo_updateParentEntryID(EntryInfo *this, const char *newParentID);
static inline void EntryInfo_updateSetParentEntryID(EntryInfo *this, const char *newParentID);

static inline char const* EntryInfo_getParentEntryID(const EntryInfo* this);
static inline char const* EntryInfo_getEntryID(const EntryInfo* this);
static inline void EntryInfo_setEntryID(EntryInfo *this, const char *entryID);
static inline void EntryInfo_setFileName(EntryInfo *this, const char* newFileName);
static inline void EntryInfo_updateFileName(EntryInfo *this, const char* newFileName);
static inline void EntryInfo_setEntryType(EntryInfo *this, DirEntryType entryType);
static inline void EntryInfo_setFlags(EntryInfo *this, int flags);

extern size_t EntryInfo_serialize(const EntryInfo* this, char* outBuf);
extern fhgfs_bool EntryInfo_deserialize(const char* buf, size_t bufLen, unsigned* outLen,
   EntryInfo* outThis);
extern unsigned EntryInfo_serialLen(const EntryInfo* this);


/**
 * minimal information about an entry (file/directory/...)
 * note: In order to properly initialize this struct, EntryInfo_init() has to be called. This
 *       is also the only function, which ever should write to the write-unprotected '_' functions.
 *       Other code is supposed to use the function without the underscore.
 */
struct EntryInfo
{
    union
    {
          const uint16_t ownerNodeID;      // nodeID of the metadata server that has the inode
          uint16_t _ownerNodeID;
    };

   union
   {
          char const* const parentEntryID; // entryID of the parent dir
          char* _parentEntryID;
   };

   union
   {
         char const* const entryID;        // entryID of the actual entry
         char* _entryID;
   };

   union
   {
         char const* const fileName;       // file/dir name of the actual entry
         char* _fileName;
   };

   union
   {
         const DirEntryType entryType;
         DirEntryType _entryType;
   };

   union
   {
         const int flags;          // additional flags (e.g. ENTRYINFO_FEATURE_INLINED)
         int _flags;
   };
};




/**
 * Main initialization function for EntryInfo, should typically be used
 *
 * @param parentEntryID will be free'd on uninit
 * @param entryID will be free'd on uninit
 * @param fileName will be free'd on uninit
 */
void EntryInfo_init(EntryInfo* this, uint16_t ownerNodeID, const char* parentEntryID,
   const char* entryID, const char* fileName, DirEntryType entryType, int flags)
{
   // TODO: Add a flag to kstrdup strings? Then set a flag for uninit to free those

   EntryInfo_setOwnerNodeID(this, ownerNodeID);
   EntryInfo_setParentEntryID(this, parentEntryID);
   EntryInfo_setEntryID(this, entryID);
   EntryInfo_setFileName(this, fileName);
   EntryInfo_setEntryType(this, entryType);
   EntryInfo_setFlags(this, flags);
}

/**
 * unitialize the object
 */
void EntryInfo_uninit(EntryInfo* this)
{
   os_kfree(this->_parentEntryID);
   os_kfree(this->_entryID);
   os_kfree(this->_fileName);
}

/**
 * @param parentEntryID will be free'd on uninit
 * @param entryID will be free'd on uninit
 * @param fileName will be free'd on uninit
 */
EntryInfo* EntryInfo_construct(uint16_t ownerNodeID, const char* parentEntryID,
   const char* entryID, const char* fileName, DirEntryType entryType, int flags)
{
   EntryInfo* this = (EntryInfo*) os_kmalloc(sizeof(*this) );
   EntryInfo_init(this, ownerNodeID, parentEntryID, entryID, fileName, entryType, flags);

   return this;
}

void EntryInfo_destruct(EntryInfo* this)
{
   EntryInfo_uninit(this);

   os_kfree(this);
}

/**
 * Duplicate inEntryInfo to outEntryInfo, also allocate memory for strings
 */
void EntryInfo_dup(const EntryInfo *inEntryInfo, EntryInfo *outEntryInfo)
{
   uint16_t    outOwnerNodeID   = inEntryInfo->ownerNodeID;
   const char* outParentEntryID = StringTk_strDup(inEntryInfo->parentEntryID);
   const char* outEntryID       = StringTk_strDup(inEntryInfo->entryID);
   const char* outFileName      = StringTk_strDup(inEntryInfo->fileName);

   DirEntryType outEntryType = inEntryInfo->entryType;
   int outflags = inEntryInfo->flags;

   EntryInfo_init(outEntryInfo, outOwnerNodeID, outParentEntryID, outEntryID, outFileName,
      outEntryType, outflags);
}

/**
 * Just copy the values of entryInfo. Caller has to be very very cautious, the memory pointers
 * will be invalid once the original object is freed!
 */
void EntryInfo_copy(const EntryInfo *inEntryInfo, EntryInfo *outEntryInfo)
{
   uint16_t    outOwnerNodeID   = inEntryInfo->ownerNodeID;
   const char* outParentEntryID = inEntryInfo->parentEntryID;
   const char* outEntryID       = inEntryInfo->entryID;
   const char* outFileName      = inEntryInfo->fileName;

   DirEntryType outEntryType = inEntryInfo->entryType;
   int outFlags = inEntryInfo->flags;

   EntryInfo_init(outEntryInfo, outOwnerNodeID, outParentEntryID, outEntryID, outFileName,
      outEntryType, outFlags);
}


/**
 * Update our EntryInfo (this) with a new EntryInfo
 *
 * Note: newEntryInfo must not be used any more by the caller
 * Note: This does not update entryID and entryType as these values cannot change. If we would
 *       update entryID we would also increase locking overhead.
 */
void EntryInfo_update(EntryInfo* this, const EntryInfo* newEntryInfo)
{
   os_kfree(this->_parentEntryID);
   os_kfree(this->_fileName);

   EntryInfo_setParentEntryID(this, newEntryInfo->parentEntryID);
   EntryInfo_setFileName(this, newEntryInfo->fileName);
   EntryInfo_setFlags(this, newEntryInfo->flags);

   if (!DirEntryType_ISDIR(this->entryType) )
   {  // only update the ownerNodeID if it is not a directory
      EntryInfo_setOwnerNodeID(this, newEntryInfo->ownerNodeID);
   }

   os_kfree(newEntryInfo->_entryID);
}

void EntryInfo_setOwnerNodeID(EntryInfo *this, uint16_t ownerNodeID)
{
   this->_ownerNodeID = ownerNodeID;
}

uint16_t EntryInfo_getOwnerNodeID(EntryInfo *this)
{
   return this->ownerNodeID;
}


void EntryInfo_setParentEntryID(EntryInfo *this, const char *parentEntryID)
{
   this->_parentEntryID = (char *) parentEntryID;
}

void EntryInfo_updateParentEntryID(EntryInfo *this, const char *newParentID)
{
   os_kfree(this->_parentEntryID);
   this->_parentEntryID = StringTk_strDup(newParentID);
}

/**
 * Note: This sets to newParentID, the caller must not use newParentID
 */
void EntryInfo_updateSetParentEntryID(EntryInfo *this, const char *newParentID)
{
   os_kfree(this->_parentEntryID);
   this->_parentEntryID = (char *) newParentID;
}

char const* EntryInfo_getParentEntryID(const EntryInfo* this)
{
   return this->parentEntryID;
}

char const* EntryInfo_getEntryID(const EntryInfo* this)
{
   return this->entryID;
}

void EntryInfo_setEntryID(EntryInfo *this, const char *entryID)
{
   this->_entryID = (char *) entryID;
}


void EntryInfo_setFileName(EntryInfo *this, const char* newFileName)
{
   this->_fileName = (char *) newFileName;
}

void EntryInfo_updateFileName(EntryInfo *this, const char* newFileName)
{
   os_kfree(this->_fileName);
   this->_fileName =  StringTk_strDup(newFileName);
}


void EntryInfo_setEntryType(EntryInfo *this, DirEntryType entryType)
{
   this->_entryType = entryType;
}

void EntryInfo_setFlags(EntryInfo *this, int flags)
{
   this->_flags = flags;
}

#endif /* ENTRYINFO_H_ */
