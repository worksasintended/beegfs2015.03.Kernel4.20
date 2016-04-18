#include "AckStoreMap.h"
#include "AckStoreMapIter.h"

AckStoreMapIter AckStoreMap_find(AckStoreMap* this, const char* searchKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem( (RBTree*)this, searchKey);
   
   AckStoreMapIter iter;
   AckStoreMapIter_init(&iter, this, treeElem);
   
   return iter;

//   return (char*)PointerRBTree_find( (RBTree*)this, searchKey);
}

AckStoreMapIter AckStoreMap_begin(AckStoreMap* this)
{
   RBTreeElem* treeElem = (RBTreeElem*)fhgfs_RBTree_first( (RBRoot*)&this->rbTree.treeroot);
   
   AckStoreMapIter iter;
   AckStoreMapIter_init(&iter, this, treeElem);
   
   return iter;

//   return (char*)PointerRBTree_find( (RBTree*)this, searchKey);
}


int compareAckStoreMapElems(const void* key1, const void* key2)
{
   return os_strcmp(key1, key2);
}



