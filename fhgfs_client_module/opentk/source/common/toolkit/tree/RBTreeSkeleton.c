#include "RBTreeSkeleton.h"


static void __rbRotateLeft(struct RBNode* node, struct RBRoot* root)
{
	struct RBNode* right = node->rb_right;

	if( (node->rb_right = right->rb_left) )
		right->rb_left->rb_parent = node;
	
	right->rb_left = node;

	if( (right->rb_parent = node->rb_parent) )
	{
		if(node == node->rb_parent->rb_left)
			node->rb_parent->rb_left = right;
		else
			node->rb_parent->rb_right = right;
	}
	else
		root->rb_node = right;
	
	node->rb_parent = right;
}

static void __rbRotateRight(struct RBNode* node, struct RBRoot* root)
{
	struct RBNode* left = node->rb_left;

	if( (node->rb_left = left->rb_right) )
		left->rb_right->rb_parent = node;
	left->rb_right = node;

	if( (left->rb_parent = node->rb_parent) )
	{
		if (node == node->rb_parent->rb_right)
			node->rb_parent->rb_right = left;
		else
			node->rb_parent->rb_left = left;
	}
	else
		root->rb_node = left;
	node->rb_parent = left;
}

void fhgfs_RBTree_insertColor(struct RBNode* node, struct RBRoot* root)
{
	struct RBNode* parent;
	struct RBNode* gparent;

	while( (parent = node->rb_parent) && parent->rb_color == RB_COL_RED)
	{
		gparent = parent->rb_parent;

		if(parent == gparent->rb_left)
		{
			{
				register struct RBNode* uncle = gparent->rb_right;
				if(uncle && uncle->rb_color == RB_COL_RED)
				{
					uncle->rb_color = RB_COL_BLACK;
					parent->rb_color = RB_COL_BLACK;
					gparent->rb_color = RB_COL_RED;
					node = gparent;
					continue;
				}
			}

			if (parent->rb_right == node)
			{
				register struct RBNode* tmp;
				__rbRotateLeft(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			parent->rb_color = RB_COL_BLACK;
			gparent->rb_color = RB_COL_RED;
			__rbRotateRight(gparent, root);
			
		}
		else
		{
			{
				register struct RBNode* uncle = gparent->rb_left;
					
				if(uncle && uncle->rb_color == RB_COL_RED)
				{
					uncle->rb_color = RB_COL_BLACK;
					parent->rb_color = RB_COL_BLACK;
					gparent->rb_color = RB_COL_RED;
					node = gparent;
					continue;
				}
			}

			if(parent->rb_left == node)
			{
				register struct RBNode* tmp;
				__rbRotateRight(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			parent->rb_color = RB_COL_BLACK;
			gparent->rb_color = RB_COL_RED;
			__rbRotateLeft(gparent, root);
		}
	}

	root->rb_node->rb_color = RB_COL_BLACK;
}



static void __rbEraseColor(
	struct RBNode* node, struct RBNode* parent, struct RBRoot *root)
{
	struct RBNode* other;

	while( (!node || node->rb_color == RB_COL_BLACK) && node != root->rb_node)
	{
		if(parent->rb_left == node)
		{
			other = parent->rb_right;
			if (other->rb_color == RB_COL_RED)
			{
				other->rb_color = RB_COL_BLACK;
				parent->rb_color = RB_COL_RED;
				__rbRotateLeft(parent, root);
				other = parent->rb_right;
			}
			if( (!other->rb_left ||
			     other->rb_left->rb_color == RB_COL_BLACK)
			    && (!other->rb_right ||
				other->rb_right->rb_color == RB_COL_BLACK))
			{
				other->rb_color = RB_COL_RED;
				node = parent;
				parent = node->rb_parent;
			}
			else
			{
				if (!other->rb_right ||
				    other->rb_right->rb_color == RB_COL_BLACK)
				{
					register struct RBNode* o_left;
					if ((o_left = other->rb_left))
						o_left->rb_color = RB_COL_BLACK;
					other->rb_color = RB_COL_RED;
					__rbRotateRight(other, root);
					other = parent->rb_right;
				}
				
				other->rb_color = parent->rb_color;
				parent->rb_color = RB_COL_BLACK;
				
				if (other->rb_right)
					other->rb_right->rb_color = RB_COL_BLACK;
				__rbRotateLeft(parent, root);
				node = root->rb_node;
				break;
			}
		}
		else
		{
			other = parent->rb_left;
			if(other->rb_color == RB_COL_RED)
			{
				other->rb_color = RB_COL_BLACK;
				parent->rb_color = RB_COL_RED;
				__rbRotateRight(parent, root);
				other = parent->rb_left;
			}
			
			if( (!other->rb_left ||
			     other->rb_left->rb_color == RB_COL_BLACK)
			    && (!other->rb_right ||
				other->rb_right->rb_color == RB_COL_BLACK))
			{
				other->rb_color = RB_COL_RED;
				node = parent;
				parent = node->rb_parent;
			}
			
			else
			{
				if(!other->rb_left ||
				    other->rb_left->rb_color == RB_COL_BLACK)
				{
					register struct RBNode* o_right;
					if ((o_right = other->rb_right))
						o_right->rb_color = RB_COL_BLACK;
					other->rb_color = RB_COL_RED;
					__rbRotateLeft(other, root);
					other = parent->rb_left;
				}
				
				other->rb_color = parent->rb_color;
				parent->rb_color = RB_COL_BLACK;
				if (other->rb_left)
					other->rb_left->rb_color = RB_COL_BLACK;
				__rbRotateRight(parent, root);
				node = root->rb_node;
				break;
			}
		}
	}
	
	if(node)
		node->rb_color = RB_COL_BLACK;
}

void fhgfs_RBTree_erase(struct RBNode* node, struct RBRoot* root)
{
	struct RBNode* child;
	struct RBNode* parent;
	rb_col_t color;

	if(!node->rb_left)
		child = node->rb_right;
	else
	if(!node->rb_right)
		child = node->rb_left;
	else
	{
		struct RBNode* old = node, *left;

		node = node->rb_right;
		while ((left = node->rb_left) != NULL)
			node = left;
		child = node->rb_right;
		parent = node->rb_parent;
		color = node->rb_color;

		if(child)
			child->rb_parent = parent;
		if (parent)
		{
			if(parent->rb_left == node)
				parent->rb_left = child;
			else
				parent->rb_right = child;
		}
		else
			root->rb_node = child;

		if(node->rb_parent == old)
			parent = node;
		
		node->rb_parent = old->rb_parent;
		node->rb_color = old->rb_color;
		node->rb_right = old->rb_right;
		node->rb_left = old->rb_left;

		if(old->rb_parent)
		{
			if(old->rb_parent->rb_left == old)
				old->rb_parent->rb_left = node;
			else
				old->rb_parent->rb_right = node;
		} else
			root->rb_node = node;

		old->rb_left->rb_parent = node;
		
		if (old->rb_right)
			old->rb_right->rb_parent = node;
		
		goto color;
	}

	parent = node->rb_parent;
	color = node->rb_color;

	if(child)
		child->rb_parent = parent;
	
	if(parent)
	{
		if (parent->rb_left == node)
			parent->rb_left = child;
		else
			parent->rb_right = child;
	}
	else
		root->rb_node = child;

 color:
	if(color == RB_COL_BLACK)
		__rbEraseColor(child, parent, root);
}



/*
 * This function returns the first node (in sort order) of the tree.
 */
struct RBNode* fhgfs_RBTree_first(struct RBRoot* root)
{
	struct RBNode* n;

	n = root->rb_node;
	if (!n)
		return NULL;
	while (n->rb_left)
		n = n->rb_left;
	return n;
}



struct RBNode* fhgfs_RBTree_last(struct RBRoot* root)
{
	struct RBNode* n;

	n = root->rb_node;
	
	if (!n)
		return NULL;
	
	while (n->rb_right)
		n = n->rb_right;
	
	return n;
}



struct RBNode* fhgfs_RBTree_next(struct RBNode* node)
{
	/* If we have a right-hand child, go down and then left as far
	   as we can. */
	if (node->rb_right)
	{
		node = node->rb_right;
		
		while (node->rb_left)
			node=node->rb_left;
		
		return node;
	}

	/* No right-hand children.  Everything down and left is
	   smaller than us, so any 'next' node must be in the general
	   direction of our parent. Go up the tree; any time the
	   ancestor is a right-hand child of its parent, keep going
	   up. First time it's a left-hand child of its parent, said
	   parent is our 'next' node. */
	while(node->rb_parent && node == node->rb_parent->rb_right)
		node = node->rb_parent;

	return node->rb_parent;
}



struct RBNode* fhgfs_RBTree_prev(struct RBNode* node)
{
	/* If we have a left-hand child, go down and then right as far
	   as we can. */
	if (node->rb_left)
	{
		node = node->rb_left; 
		while (node->rb_right)
			node=node->rb_right;
		
		return node;
	}

	/* No left-hand children. Go up till we find an ancestor which
	   is a right-hand child of its parent */
	while (node->rb_parent && node == node->rb_parent->rb_left)
		node = node->rb_parent;

	return node->rb_parent;
}



void fhgfs_RBTree_replaceNode(struct RBNode* victim, struct RBNode* new, struct RBRoot* root)
{
	struct RBNode* parent = victim->rb_parent;

	/* Set the surrounding nodes to point to the replacement */
	if(parent)
	{
		if (victim == parent->rb_left)
			parent->rb_left = new;
		else
			parent->rb_right = new;
	}
	else
	{
		root->rb_node = new;
	}
	
	if (victim->rb_left)
		victim->rb_left->rb_parent = new;
	if (victim->rb_right)
		victim->rb_right->rb_parent = new;

	/* Copy the pointers/colour from the victim to the replacement */
	*new = *victim;
}


