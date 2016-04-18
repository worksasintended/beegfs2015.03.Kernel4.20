#include "NodeMap.h"
#include "NodeMapIter.h"

NodeMapIter NodeMap_find(NodeMap* this, uint16_t searchKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem( (RBTree*)this, (void*)(size_t)searchKey);

   NodeMapIter iter;
   NodeMapIter_init(&iter, this, treeElem);

   return iter;
}

NodeMapIter NodeMap_begin(NodeMap* this)
{
   RBTreeElem* treeElem = (RBTreeElem*)fhgfs_RBTree_first( (RBRoot*)&this->rbTree.treeroot);

   NodeMapIter iter;
   NodeMapIter_init(&iter, this, treeElem);

   return iter;
}


int compareNodeMapElems(const void* key1, const void* key2)
{
   if(key1 < key2)
      return -1;
   else
   if(key1 > key2)
      return 1;
   else
      return 0;
}



