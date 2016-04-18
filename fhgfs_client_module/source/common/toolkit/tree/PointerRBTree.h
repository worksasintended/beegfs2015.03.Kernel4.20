#ifndef POINTERRBTREE_H_
#define POINTERRBTREE_H_

#include <common/Common.h>
#include <opentk/common/toolkit/tree/OpenTk_RBTreeSkeleton.h>


struct RBTreeElem;
typedef struct RBTreeElem RBTreeElem;
struct RBTreeRoot;
typedef struct RBTreeRoot RBTreeRoot;
struct RBTree;
typedef struct RBTree RBTree;


/**
 * @return: <0 if key1<key2, 0 for equal keys, >0 otherwise
 */
typedef int (*TreeElemsComparator)(const void* key1, const void* key2);


struct RBTreeIter; // forward declaration of the iterator

static inline void PointerRBTree_init(RBTree* this, TreeElemsComparator compareTreeElems);
static inline RBTree* PointerRBTree_construct(TreeElemsComparator compareTreeElems);
static inline void PointerRBTree_uninit(RBTree* this);
static inline void PointerRBTree_destruct(RBTree* this);

static inline RBTreeElem* _PointerRBTree_findElem(RBTree* this, const void* searchKey);

static inline fhgfs_bool PointerRBTree_insert(RBTree* this, void* newKey, void* newValue);
static fhgfs_bool PointerRBTree_erase(RBTree* this, const void* eraseKey);
static inline size_t PointerRBTree_length(RBTree* this);

static inline void PointerRBTree_clear(RBTree* this);

extern struct RBTreeIter PointerRBTree_find(RBTree* this, const void* searchKey);
extern struct RBTreeIter PointerRBTree_begin(RBTree* this);

struct RBTreeElem
{
   RBNode treenode;

   void* key;
   void* value;
};

struct RBTreeRoot
{
   RBTreeElem *rootnode;
};


struct RBTree
{
   RBTreeRoot treeroot;

   size_t length;

   TreeElemsComparator compareTreeElems;
};


void PointerRBTree_init(RBTree* this, TreeElemsComparator compareTreeElems)
{
   this->treeroot.rootnode = NULL;
   this->length = 0;

   this->compareTreeElems = compareTreeElems;
}

RBTree* PointerRBTree_construct(TreeElemsComparator compareTreeElems)
{
   RBTree* this = (RBTree*)os_kmalloc(sizeof(RBTree) );

   PointerRBTree_init(this, compareTreeElems);

   return this;
}

void PointerRBTree_uninit(RBTree* this)
{
   PointerRBTree_clear(this);
}

void PointerRBTree_destruct(RBTree* this)
{
   PointerRBTree_uninit(this);

   os_kfree(this);
}


/**
 * @return NULL if the element was not found
 */
RBTreeElem* _PointerRBTree_findElem(RBTree* this, const void* searchKey)
{
   int compRes;

   RBNode* node = (RBNode*)this->treeroot.rootnode;

   while(node)
   {
      RBTreeElem* treeElem = (RBTreeElem*)node;

      compRes = this->compareTreeElems(searchKey, treeElem->key);

      if(compRes < 0)
         node = node->rb_left;
      else
      if(compRes > 0)
         node = node->rb_right;
      else
      { // element found => return it
         return treeElem;
      }
   }

   // element not found

   return NULL;
}


/**
 * @return fhgfs_true if element was inserted, fhgfs_false if it already existed (in which case
 * nothing is changed) or if out of mem.
 */
fhgfs_bool PointerRBTree_insert(RBTree* this, void* newKey, void* newValue)
{
   RBTreeElem* newElem;

   // the parent's (left or right) link to which the child will be connected
   RBNode** link = (RBNode**)&this->treeroot.rootnode;
   // the parent of the new node
   RBNode* parent = NULL;

   while(*link)
   {
      int compRes;
      RBTreeElem* treeElem;

      parent = *link;
      treeElem = (RBTreeElem*)parent;

      compRes = this->compareTreeElems(newKey, treeElem->key);

      if(compRes < 0)
         link = &(*link)->rb_left;
      else
      if(compRes > 0)
         link = &(*link)->rb_right;
      else
      { // target already exists => do nothing (according to the behavior of c++ map::insert)
         //rbReplaceNode(parent, newElem, this);

         return fhgfs_false;
      }
   }

   // create new element

   newElem = (RBTreeElem*)os_kmalloc(sizeof(RBTreeElem) );
   if(unlikely(!newElem) )
   {
      printk_fhgfs(KERN_WARNING, "%s:%d: Memory allocation failed. Dumping stack...\n",
         __func__, __LINE__);
      os_dumpStack();

      return fhgfs_false; // return code has potential for improvement
   }

   newElem->key = newKey;
   newElem->value = newValue;

   // link the new element
   fhgfs_RBTree_linkNode( (RBNode*)newElem, parent, link);

   this->length++;

   return fhgfs_true;
}

/**
 * @return fhgfs_false if no element with the given key existed
 */
fhgfs_bool PointerRBTree_erase(RBTree* this, const void* eraseKey)
{
   RBTreeElem* treeElem;

   treeElem = _PointerRBTree_findElem(this, eraseKey);
   if(!treeElem)
   { // element not found
      return fhgfs_false;
   }

   // unlink the element
   fhgfs_RBTree_erase( (RBNode*)treeElem, (RBRoot*)&this->treeroot);

   os_kfree(treeElem);

   this->length--;

   return fhgfs_true;
}

size_t PointerRBTree_length(RBTree* this)
{
   return this->length;
}

void PointerRBTree_clear(RBTree* this)
{
   while(this->length)
   {
      PointerRBTree_erase(this, this->treeroot.rootnode->key);
   }
}

#endif /*POINTERRBTREE_H_*/
