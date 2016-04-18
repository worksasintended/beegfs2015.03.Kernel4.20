#ifndef OPENTK_RBTREESKELETON_H_
#define OPENTK_RBTREESKELETON_H_

#include <opentk/OpenTk_Common.h>

/*
  Based upon Kernel Red Black Trees (linux/include/linux/rbtree.h)
  The corresponding documentation says:

  To use rbtrees you'll have to implement your own insert and search cores.
  This will avoid us to use callbacks and to drop drammatically performances.
  I know it's not the cleaner way,  but in C (not in C++) to get
  performances and genericity...

  Some example of insert and search follows here. The search is a plain
  normal search over an ordered tree. The insert instead must be implemented
  int two steps: as first thing the code must insert the element in
  order as a red leaf in the tree, then the support library function
  rb_insert_color() must be called. Such function will do the
  not trivial work to rebalance the rbtree if necessary.

-----------------------------------------------------------------------
static inline struct page * rb_search_page_cache(struct inode * inode,
						 unsigned long offset)
{
	struct rb_node * n = inode->i_rb_page_cache.rb_node;
	struct page * page;

	while (n)
	{
		page = rb_entry(n, struct page, rb_page_cache);

		if (offset < page->offset)
			n = n->rb_left;
		else if (offset > page->offset)
			n = n->rb_right;
		else
			return page;
	}
	return NULL;
}

static inline struct page * __rb_insert_page_cache(struct inode * inode,
						   unsigned long offset,
						   struct rb_node * node)
{
	struct rb_node ** p = &inode->i_rb_page_cache.rb_node;
	struct rb_node * parent = NULL;
	struct page * page;

	while (*p)
	{
		parent = *p;
		page = rb_entry(parent, struct page, rb_page_cache);

		if (offset < page->offset)
			p = &(*p)->rb_left;
		else if (offset > page->offset)
			p = &(*p)->rb_right;
		else
			return page;
	}

	rb_link_node(node, parent, p);

	return NULL;
}

static inline struct page * rb_insert_page_cache(struct inode * inode,
						 unsigned long offset,
						 struct rb_node * node)
{
	struct page * ret;
	if( (ret = __rb_insert_page_cache(inode, offset, node) ) )
		goto out;
	rb_insert_color(node, &inode->i_rb_page_cache);
 out:
	return ret;
}
-----------------------------------------------------------------------
*/

enum rb_col
{
	RB_COL_RED = 0,
	RB_COL_BLACK = 1
};
typedef enum rb_col rb_col_t;

struct RBNode
{
	struct RBNode *rb_parent;
	struct RBNode *rb_right;
	struct RBNode *rb_left;

	rb_col_t rb_color;
};
typedef struct RBNode RBNode;

struct RBRoot
{
	struct RBNode *rb_node;
};
typedef struct RBRoot RBRoot;


#define RBNODE_ROOT	(struct RBRoot) {NULL, }
//#define	rbEntry(ptr, type, member) container_of(ptr, type, member)
#define rbEntry(ptr)	( (struct RBNode*)ptr)


extern void fhgfs_RBTree_insertColor(struct RBNode* node, struct RBRoot* root);
extern void fhgfs_RBTree_erase(struct RBNode* node, struct RBRoot* root);

/* Find logical next and previous nodes in a tree */
extern struct RBNode *fhgfs_RBTree_first(struct RBRoot* root);
extern struct RBNode *fhgfs_RBTree_last(struct RBRoot* root);
extern struct RBNode *fhgfs_RBTree_next(struct RBNode* node);
extern struct RBNode *fhgfs_RBTree_prev(struct RBNode* node);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
extern void fhgfs_RBTree_replaceNode(struct RBNode* victim, struct RBNode* new,
   struct RBRoot* root);


static inline void fhgfs_RBTree_linkNode(struct RBNode* node, struct RBNode* parent,
   struct RBNode** rb_link)
{
   node->rb_parent = parent;
   node->rb_color = RB_COL_RED;
   node->rb_left = node->rb_right = NULL;

   *rb_link = node;
}



#endif // OPENTK_RBTREESKELETON_H_
