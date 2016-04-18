#include <common/toolkit/tree/TargetStateInfoMap.h>
#include <common/toolkit/tree/TargetStateInfoMapIter.h>

TargetStateInfoMapIter TargetStateInfoMap_find(TargetStateInfoMap* this,
   const uint16_t searchKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem(
      (RBTree*)this, (const void*)(const size_t)searchKey);

   TargetStateInfoMapIter iter;
   TargetStateInfoMapIter_init(&iter, this, treeElem);

   return iter;
}

TargetStateInfoMapIter TargetStateInfoMap_begin(TargetStateInfoMap* this)
{
   RBTreeElem* treeElem = (RBTreeElem*)fhgfs_RBTree_first( (RBRoot*)&this->rbTree.treeroot);

   TargetStateInfoMapIter iter;
   TargetStateInfoMapIter_init(&iter, this, treeElem);

   return iter;
}


int compareTargetStateInfoMapElems(const void* key1, const void* key2)
{
   if(key1 < key2)
      return -1;
   else
   if(key1 > key2)
      return 1;
   else
      return 0;
}

