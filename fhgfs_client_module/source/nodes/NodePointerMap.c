#include "NodePointerMap.h"
#include "NodePointerMapIter.h"

NodePointerMapIter NodePointerMap_find(NodePointerMap* this, Node* searchKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem( (RBTree*)this, searchKey);

   NodePointerMapIter iter;
   NodePointerMapIter_init(&iter, this, treeElem);

   return iter;
}

NodePointerMapIter NodePointerMap_begin(NodePointerMap* this)
{
   RBTreeElem* treeElem = (RBTreeElem*)fhgfs_RBTree_first( (RBRoot*)&this->rbTree.treeroot);

   NodePointerMapIter iter;
   NodePointerMapIter_init(&iter, this, treeElem);

   return iter;
}


int compareNodePointerMapElems(const void* key1, const void* key2)
{
   if(key1 < key2)
      return -1;
   else
   if(key1 > key2)
      return 1;
   else
      return 0;
}



