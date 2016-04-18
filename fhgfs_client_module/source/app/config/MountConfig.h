#ifndef OPEN_MOUNTCONFIG_H_
#define OPEN_MOUNTCONFIG_H_

#include <common/Common.h>

struct MountConfig;
typedef struct MountConfig MountConfig;


static inline void MountConfig_init(MountConfig* this);
static inline MountConfig* MountConfig_construct(void);
static inline void MountConfig_uninit(MountConfig* this);
static inline void MountConfig_destruct(MountConfig* this);

extern fhgfs_bool MountConfig_parseFromRawOptions(MountConfig* this, char* mountOptions);


struct MountConfig
{
   char* cfgFile;
   char* logStdFile;
   char* logErrFile;
   char* sysMgmtdHost;
   char* tunePreferredMetaFile;
   char* tunePreferredStorageFile;

   fhgfs_bool logLevelDefined; // fhgfs_true if the value has been specified
   fhgfs_bool connPortShiftDefined; // fhgfs_true if the value has been specified
   fhgfs_bool connMgmtdPortUDPDefined; // fhgfs_true if the value has been specified
   fhgfs_bool connMgmtdPortTCPDefined; // fhgfs_true if the value has been specified
   fhgfs_bool sysMountSanityCheckMSDefined; // fhgfs_true if the value has been specified

   int logLevel;
   unsigned connPortShift;
   unsigned connMgmtdPortUDP;
   unsigned connMgmtdPortTCP;
   unsigned sysMountSanityCheckMS;
};


void MountConfig_init(MountConfig* this)
{
   os_memset(this, 0, sizeof(*this) );
}

struct MountConfig* MountConfig_construct(void)
{
   struct MountConfig* this = (MountConfig*)os_kmalloc(sizeof(*this) );

   MountConfig_init(this);

   return this;
}

void MountConfig_uninit(MountConfig* this)
{
   SAFE_KFREE(this->cfgFile);
   SAFE_KFREE(this->logStdFile);
   SAFE_KFREE(this->logErrFile);
   SAFE_KFREE(this->sysMgmtdHost);
   SAFE_KFREE(this->tunePreferredMetaFile);
   SAFE_KFREE(this->tunePreferredStorageFile);
}

void MountConfig_destruct(MountConfig* this)
{
   MountConfig_uninit(this);

   os_kfree(this);
}


#endif /*OPEN_MOUNTCONFIG_H_*/
