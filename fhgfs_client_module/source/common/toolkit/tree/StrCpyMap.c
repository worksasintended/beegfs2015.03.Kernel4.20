#include "StrCpyMap.h"
#include "StrCpyMapIter.h"

StrCpyMapIter StrCpyMap_find(StrCpyMap* this, const char* searchKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem( (RBTree*)this, searchKey);
   
   StrCpyMapIter iter;
   StrCpyMapIter_init(&iter, this, treeElem);
   
   return iter;

//   return (char*)PointerRBTree_find( (RBTree*)this, searchKey);
}

StrCpyMapIter StrCpyMap_begin(StrCpyMap* this)
{
   RBTreeElem* treeElem = (RBTreeElem*)fhgfs_RBTree_first( (RBRoot*)&this->rbTree.treeroot);
   
   StrCpyMapIter iter;
   StrCpyMapIter_init(&iter, this, treeElem);
   
   return iter;
}


int compareStrCpyMapElems(const void* key1, const void* key2)
{
   return os_strcmp(key1, key2);
}

