#ifndef NETFILTER_H_
#define NETFILTER_H_

#include <app/config/Config.h>
#include <common/toolkit/Serialization.h>
#include <common/toolkit/SocketTk.h>
#include <common/Common.h>


struct NetFilterEntry;
typedef struct NetFilterEntry NetFilterEntry;

struct NetFilter;
typedef struct NetFilter NetFilter;


static inline void NetFilter_init(NetFilter* this, const char* filename);
static inline NetFilter* NetFilter_construct(const char* filename);
static inline void NetFilter_uninit(NetFilter* this);
static inline void NetFilter_destruct(NetFilter* this);

// inliners
static inline fhgfs_bool NetFilter_isAllowed(NetFilter* this, fhgfs_in_addr ipAddr);
static inline fhgfs_bool NetFilter_isContained(NetFilter* this, fhgfs_in_addr ipAddr);
static inline fhgfs_bool __NetFilter_prepareArray(NetFilter* this, const char* filename);

// getters & setters
static inline fhgfs_bool NetFilter_getValid(NetFilter* this);
static inline size_t NetFilter_getNumFilterEntries(NetFilter* this);


struct NetFilterEntry
{
   fhgfs_in_addr netAddressShifted; // shifted by number of significant bits
      // Note: this address is in host(!) byte order to enable correct shift operator usage
   int shiftBitsNum; // for right shift
};

struct NetFilter
{
   NetFilterEntry* filterArray;
   size_t filterArrayLen;

   fhgfs_bool isValid;
};


/**
 * Note: Call isValid() to find out whether initialization was successfull.
 *
 * @param filename path to filter file.
 */
void NetFilter_init(NetFilter* this, const char* filename)
{
   this->isValid = fhgfs_true;

   if(!__NetFilter_prepareArray(this, filename) )
      this->isValid = fhgfs_false;
}

NetFilter* NetFilter_construct(const char* filename)
{
   NetFilter* this = (NetFilter*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      NetFilter_init(this, filename);

   return this;
}

void NetFilter_uninit(NetFilter* this)
{
   SAFE_KFREE(this->filterArray);
}

void NetFilter_destruct(NetFilter* this)
{
   NetFilter_uninit(this);

   os_kfree(this);
}

/**
 * Note: Empty filter will always return allowed (i.e. "true").
 * Note: Will always allow the loopback address.
 */
fhgfs_bool NetFilter_isAllowed(NetFilter* this, fhgfs_in_addr ipAddr)
{
   if(!this->filterArrayLen)
      return fhgfs_true;

   return NetFilter_isContained(this, ipAddr);
}

/**
 * Note: Always implicitly contains the loopback address.
 */
fhgfs_bool NetFilter_isContained(NetFilter* this, fhgfs_in_addr ipAddr)
{
   fhgfs_in_addr ipHostOrder;
   size_t i;

   // note: stored addresses are in host byte order to enable correct shift operator usage
   ipHostOrder = Serialization_ntohlTrans(ipAddr);

   if(ipHostOrder == BEEGFS_INADDR_LOOPBACK) // (inaddr_loopback is in host byte order)
      return fhgfs_true;

   for(i = 0; i < this->filterArrayLen; i++)
   {
      const fhgfs_in_addr ipHostOrderShifted =
         ipHostOrder >> ( (this->filterArray)[i].shiftBitsNum);

      if( (this->filterArray)[i].netAddressShifted == ipHostOrderShifted)
      { // address match
         return fhgfs_true;
      }
   }

   // no match found
   return fhgfs_false;
}

/**
 * @param filename path to filter file.
 * @return fhgfs_false if a specified file could not be opened.
 */
fhgfs_bool __NetFilter_prepareArray(NetFilter* this, const char* filename)
{
   fhgfs_bool loadRes = fhgfs_true;
   StrCpyList filterList;

   this->filterArray = NULL;
   this->filterArrayLen = 0;

   if(!StringTk_hasLength(filename) )
   { // no file specified => no filter entries
      return fhgfs_true;
   }


   StrCpyList_init(&filterList);

   loadRes = Config_loadStringListFile(filename, &filterList);
   if(!loadRes)
   { // file error
      printk_fhgfs(KERN_WARNING,
         "Unable to load configured net filter file: %s\n", filename);
   }
   else
   if(StrCpyList_length(&filterList) )
   { // file did contain some lines
      StrCpyListIter iter;
      size_t i;

      this->filterArrayLen = StrCpyList_length(&filterList);
      this->filterArray =
         (NetFilterEntry*)os_kmalloc(this->filterArrayLen * sizeof(NetFilterEntry) );

      StrCpyListIter_init(&iter, &filterList);
      i = 0;

      for( ; !StrCpyListIter_end(&iter); StrCpyListIter_next(&iter) )
      {
         char* line = StrCpyListIter_value(&iter);
         fhgfs_in_addr entryAddr;

         char* findRes = os_strchr(line, '/');
         if(!findRes)
         { // slash character missing => ignore this faulty line (and reduce filter length)
            this->filterArrayLen--;
            continue;
         }

         *findRes = 0;

         // note: we store addresses in host byte order to enable correct shift operator usage
         entryAddr = Serialization_ntohlTrans(SocketTk_in_aton(line) );
         (this->filterArray)[i].shiftBitsNum = 32 - StringTk_strToUInt(findRes + 1);
         (this->filterArray)[i].netAddressShifted =
            entryAddr >> ( (this->filterArray)[i].shiftBitsNum);

         i++; // must be increased here because of the faulty line handling
      }

      StrCpyListIter_uninit(&iter);
   }

   StrCpyList_uninit(&filterList);

   return loadRes;
}

fhgfs_bool NetFilter_getValid(NetFilter* this)
{
   return this->isValid;
}

size_t NetFilter_getNumFilterEntries(NetFilter* this)
{
   return this->filterArrayLen;
}


#endif /* NETFILTER_H_ */
