#include <common/toolkit/tree/MirrorBuddyGroupMap.h>
#include <common/toolkit/tree/MirrorBuddyGroupMapIter.h>

MirrorBuddyGroupMapIter MirrorBuddyGroupMap_find(MirrorBuddyGroupMap* this,
   const uint16_t searchKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem(
      (RBTree*)this, (const void*)(const size_t)searchKey);

   MirrorBuddyGroupMapIter iter;
   MirrorBuddyGroupMapIter_init(&iter, this, treeElem);

   return iter;
}

MirrorBuddyGroupMapIter MirrorBuddyGroupMap_begin(MirrorBuddyGroupMap* this)
{
   RBTreeElem* treeElem = (RBTreeElem*)fhgfs_RBTree_first( (RBRoot*)&this->rbTree.treeroot);

   MirrorBuddyGroupMapIter iter;
   MirrorBuddyGroupMapIter_init(&iter, this, treeElem);

   return iter;
}


int compareMirrorBuddyGroupMapElems(const void* key1, const void* key2)
{
   if(key1 < key2)
      return -1;
   else
   if(key1 > key2)
      return 1;
   else
      return 0;
}

