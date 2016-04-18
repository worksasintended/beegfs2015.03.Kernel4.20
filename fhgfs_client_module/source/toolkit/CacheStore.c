#include "CacheStore.h"

/**
 * Compares keys of the CacheStore.
 * 
 * @return: <0 if key1<key2, 0 for equal keys, >0 otherwise
 */
int __CacheStore_keyComparator(const void* key1, const void* key2)
{
   if(key1 < key2)
      return -1;
   
   if(key1 > key2)
      return 1;
   
   return 0;
}
