#include <common/nodes/ConnectionListIter.h>
#include <common/net/sock/NicAddressListIter.h>
#include <common/toolkit/list/IntCpyListIter.h>
#include <common/toolkit/list/StrCpyListIter.h>
#include <common/toolkit/list/UInt16ListIter.h>
#include <common/toolkit/tree/PointerRBTree.h>
#include <common/toolkit/tree/PointerRBTreeIter.h>
#include "ListTk.h"


/**
 * @return NULL if out of memory.
 */
NicAddressList* ListTk_cloneNicAddressList(NicAddressList* nicList)
{
   NicAddressListIter iter;
   NicAddressList* nicListClone;
   
   nicListClone = NicAddressList_construct();
   
   if(unlikely(!nicListClone) )
      return NULL;

   NicAddressListIter_init(&iter, nicList);
   
   for( ; !NicAddressListIter_end(&iter); NicAddressListIter_next(&iter) )
   {
      NicAddress* nicAddr = NicAddressListIter_value(&iter);
      NicAddress* nicAddrClone;
      
      // clone element

      nicAddrClone = (NicAddress*)os_kmalloc(sizeof(NicAddress) );
      if(unlikely(!nicAddrClone) )
         goto err_cleanup_listiter;

      os_memcpy(nicAddrClone, nicAddr, sizeof(NicAddress) );
      
      // append to the clone list
      NicAddressList_append(nicListClone, nicAddrClone);
   }

   NicAddressListIter_uninit(&iter);
   
   if(unlikely(NicAddressList_length(nicList) != NicAddressList_length(nicListClone) ) )
      goto err_cleanup_list;

   return nicListClone;   


   // memory allocation for a list element failed

err_cleanup_listiter:
   NicAddressListIter_uninit(&iter);

err_cleanup_list:
   ListTk_kfreeNicAddressListElems(nicListClone);
   NicAddressList_destruct(nicListClone);

   return NULL;
}

/**
 * Returns a cloned version of a list with only standard NICs in it.
 * (Non-standard elements in the source will just be ignored.)
 */ 
NicAddressList* ListTk_cloneStandardNicAddressList(NicAddressList* nicList)
{
   NicAddressListIter iter;
   NicAddressList* nicListClone;
   
   nicListClone = NicAddressList_construct();
   
   NicAddressListIter_init(&iter, nicList);
   
   for( ; !NicAddressListIter_end(&iter); NicAddressListIter_next(&iter) )
   {
      NicAddress* nicAddr = NicAddressListIter_value(&iter);
      NicAddress* nicAddrClone;
      
      // ignore non-standard NICs
      if(nicAddr->nicType != NICADDRTYPE_STANDARD)
         continue;
      
      // clone
      nicAddrClone = (NicAddress*)os_kmalloc(sizeof(NicAddress) );
      os_memcpy(nicAddrClone, nicAddr, sizeof(NicAddress) );
      
      // append to the clone list
      NicAddressList_append(nicListClone, nicAddrClone);
   }

   NicAddressListIter_uninit(&iter);
   
   return nicListClone;   
}


/**
 * Returns a sorted clone of a nicList.
 * 
 * Note: As this is based on a tree insertion, it won't work when there are duplicate elements
 * in the list.
 */
NicAddressList* ListTk_cloneSortNicAddressList(NicAddressList* nicList)
{
   NicAddressListIter listIter;
   NicAddressList* nicListClone;
   RBTree tree;
   RBTreeIter treeIter;
   
   
   PointerRBTree_init(&tree, NicAddress_treeComparator);   
   
   // sort => insert each list elem into the tree (no copying)
   
   NicAddressListIter_init(&listIter, nicList);
   
   for( ; !NicAddressListIter_end(&listIter); NicAddressListIter_next(&listIter) )
   {
      NicAddress* nicAddr = NicAddressListIter_value(&listIter);
      
      // Note: We insert elems into the tree without copying, so there's no special cleanup
      //    required for the tree when we're done
      PointerRBTree_insert(&tree, nicAddr, NULL);
   }

   NicAddressListIter_uninit(&listIter);
   
   
   // copy elements from tree to clone list (sorted)
   
   nicListClone = NicAddressList_construct();
   
   treeIter = PointerRBTree_begin(&tree);
   
   for( ; !PointerRBTreeIter_end(&treeIter); PointerRBTreeIter_next(&treeIter) )
   {
      NicAddress* nicAddr = (NicAddress*)PointerRBTreeIter_key(&treeIter);
      
      // clone
      NicAddress* nicAddrClone = (NicAddress*)os_kmalloc(sizeof(NicAddress) );
      os_memcpy(nicAddrClone, nicAddr, sizeof(NicAddress) );

      // append to the clone list
      NicAddressList_append(nicListClone, nicAddrClone);
   }

   PointerRBTreeIter_uninit(&treeIter);

   
   PointerRBTree_uninit(&tree);   
   
   return nicListClone;   
}

void ListTk_kfreeNicAddressListElems(NicAddressList* nicList)
{
   NicAddressListIter nicIter;
   
   NicAddressListIter_init(&nicIter, nicList);
   
   for( ; !NicAddressListIter_end(&nicIter); NicAddressListIter_next(&nicIter) )
   {
      NicAddress* nicAddr = NicAddressListIter_value(&nicIter);
      os_kfree(nicAddr);      
   }
   
   NicAddressListIter_uninit(&nicIter);
}

void ListTk_copyStrCpyListToVec(StrCpyList* srcList, StrCpyVec* destVec)
{
   StrCpyListIter listIter;
   
   StrCpyListIter_init(&listIter, srcList);
   
   for( ; !StrCpyListIter_end(&listIter); StrCpyListIter_next(&listIter) )
   {
      char* currentElem = StrCpyListIter_value(&listIter);
      
      StrCpyVec_append(destVec, currentElem);
   }

   StrCpyListIter_uninit(&listIter);
}

void ListTk_copyIntCpyListToVec(IntCpyList* srcList, IntCpyVec* destVec)
{
   IntCpyListIter listIter;
   
   IntCpyListIter_init(&listIter, srcList);
   
   for( ; !IntCpyListIter_end(&listIter); IntCpyListIter_next(&listIter) )
   {
      int currentElem = IntCpyListIter_value(&listIter);
      
      IntCpyVec_append(destVec, currentElem);
   }

   IntCpyListIter_uninit(&listIter);
}

void ListTk_copyUInt16ListToVec(UInt16List* srcList, UInt16Vec* destVec)
{
   UInt16ListIter listIter;

   UInt16ListIter_init(&listIter, srcList);

   for( ; !UInt16ListIter_end(&listIter); UInt16ListIter_next(&listIter) )
   {
      int currentElem = UInt16ListIter_value(&listIter);

      UInt16Vec_append(destVec, currentElem);
   }

   UInt16ListIter_uninit(&listIter);
}


/**
 * @param outPos zero-based list position of the searchStr if found, undefined otherwise
 */
fhgfs_bool ListTk_listContains(char* searchStr, StrCpyList* list, ssize_t* outPos)
{
   StrCpyListIter iter;
   
   (*outPos) = 0;
   
   StrCpyListIter_init(&iter, list);
   
   for( ; !StrCpyListIter_end(&iter); StrCpyListIter_next(&iter) )
   {
      char* currentElem = StrCpyListIter_value(&iter);
      
      if(!os_strcmp(searchStr, currentElem) )
      { // we found it
         StrCpyListIter_uninit(&iter);
         
         return fhgfs_true;
      }
      
      (*outPos)++;
   }

   StrCpyListIter_uninit(&iter);
   
   (*outPos) = -1;
   
   return fhgfs_false;
}


