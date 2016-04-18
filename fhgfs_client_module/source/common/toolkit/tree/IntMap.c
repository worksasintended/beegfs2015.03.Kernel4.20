#include <common/toolkit/tree/IntMap.h>
#include <common/toolkit/tree/IntMapIter.h>

IntMapIter IntMap_find(IntMap* this, const int searchKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem(
      (RBTree*)this, (const void*)(const size_t)searchKey);

   IntMapIter iter;
   IntMapIter_init(&iter, this, treeElem);

   return iter;
}

IntMapIter IntMap_begin(IntMap* this)
{
   RBTreeElem* treeElem = (RBTreeElem*)fhgfs_RBTree_first( (RBRoot*)&this->rbTree.treeroot);

   IntMapIter iter;
   IntMapIter_init(&iter, this, treeElem);

   return iter;
}


int compareIntMapElems(const void* key1, const void* key2)
{
   if(key1 < key2)
      return -1;
   else
   if(key1 > key2)
      return 1;
   else
      return 0;
}

