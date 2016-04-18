#include <common/toolkit/tree/UInt16Map.h>
#include <common/toolkit/tree/UInt16MapIter.h>


UInt16MapIter UInt16Map_find(UInt16Map* this, const uint16_t searchKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem(
      (RBTree*)this, (const void*)(const size_t)searchKey);

   UInt16MapIter iter;
   UInt16MapIter_init(&iter, this, treeElem);

   return iter;
}

UInt16MapIter UInt16Map_begin(UInt16Map* this)
{
   RBTreeElem* treeElem = (RBTreeElem*)fhgfs_RBTree_first( (RBRoot*)&this->rbTree.treeroot);

   UInt16MapIter iter;
   UInt16MapIter_init(&iter, this, treeElem);

   return iter;
}


int compareUInt16MapElems(const void* key1, const void* key2)
{
   if(key1 < key2)
      return -1;
   else
   if(key1 > key2)
      return 1;
   else
      return 0;
}

