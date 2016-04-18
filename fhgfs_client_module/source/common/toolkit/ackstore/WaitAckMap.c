#include "WaitAckMap.h"
#include "WaitAckMapIter.h"

WaitAckMapIter WaitAckMap_find(WaitAckMap* this, const char* searchKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem( (RBTree*)this, searchKey);
   
   WaitAckMapIter iter;
   WaitAckMapIter_init(&iter, this, treeElem);
   
   return iter;

//   return (char*)PointerRBTree_find( (RBTree*)this, searchKey);
}

WaitAckMapIter WaitAckMap_begin(WaitAckMap* this)
{
   RBTreeElem* treeElem = (RBTreeElem*)fhgfs_RBTree_first( (RBRoot*)&this->rbTree.treeroot);
   
   WaitAckMapIter iter;
   WaitAckMapIter_init(&iter, this, treeElem);
   
   return iter;

//   return (char*)PointerRBTree_find( (RBTree*)this, searchKey);
}


int compareWaitAckMapElems(const void* key1, const void* key2)
{
   return os_strcmp(key1, key2);
}



