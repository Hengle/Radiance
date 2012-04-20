// RedBlackNodeTree.cpp
// A red & black binary tree.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "IntContainer.h"
#include "RedBlackNodeTree.h"


namespace container {

#if defined(RAD_OPT_DEBUG)

static const char* STDPTR_DEBUG_ERROR_BADMEMORY = "Bad Pointer / Memory Corruption Detected!";

#endif

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// RedBlackNodeTree::Node
//
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::Node::Node()
//////////////////////////////////////////////////////////////////////////////////////////

RedBlackNodeTree::Node::Node()
{
#if defined (RAD_OPT_DEBUG)
	m_magicID = MAGIC_ID;
	m_inTree = NULL;

	m_parent = m_leftChild = m_rightChild = NULL;
#endif
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::Node::~Node()
//////////////////////////////////////////////////////////////////////////////////////////

RedBlackNodeTree::Node::~Node()
{
	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);
	RAD_ASSERT(!m_inTree);
}




//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// RedBlackNodeTree
//
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::RedBlackNodeTree()
//////////////////////////////////////////////////////////////////////////////////////////

RedBlackNodeTree::RedBlackNodeTree()
{
#if defined (RAD_OPT_DEBUG)
	m_magicID = MAGIC_ID;
#endif

	m_NIL = &m_nilStorage;
	m_root = &m_rootStorage;

	m_NIL->m_leftChild = m_NIL->m_rightChild = m_NIL->m_parent = m_NIL;
	m_NIL->m_red = false;

	//////////////////////////////////////////////////////////////////////////////////////////
	// predicate::Compare< typename _ARG0_TYPE, typename _ARG1_TYPE, typename _ARG0_ADAPTER, typename _ARG1_ADAPTER >
	//////////////////////////////////////////////////////////////////////////////////////////

	m_root->m_parent = m_root->m_leftChild = m_root->m_rightChild = m_NIL;
	m_root->m_red = false;

	m_size = 0;
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::~RedBlackNodeTree()
//////////////////////////////////////////////////////////////////////////////////////////

RedBlackNodeTree::~RedBlackNodeTree()
{
	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);
	RAD_ASSERT_MSG(IsEmpty(), "Attempting to destruct an active RedBlackTree!");
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::Clear()
//////////////////////////////////////////////////////////////////////////////////////////

void RedBlackNodeTree::Clear()
{
	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);

#if defined (RAD_OPT_DEBUG)
	{
		Node* n = First();

		while (n != NULL)
		{
			Node* p = Next(n);

			n->m_inTree = NULL;

			n = p;
		}
	}
#endif

	m_NIL->m_leftChild = m_NIL->m_rightChild = m_NIL->m_parent = m_NIL;
	m_NIL->m_red = false;

	m_root->m_parent = m_root->m_leftChild = m_root->m_rightChild = m_NIL;
	m_root->m_red = false;

	m_size = 0;
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::LeftRotate()
//////////////////////////////////////////////////////////////////////////////////////////

void RedBlackNodeTree::LeftRotate(Node* x)
{
	RAD_ASSERT(x);
	RAD_ASSERT(x->m_magicID == Node::MAGIC_ID);
	RAD_ASSERT(x->m_inTree == this);

	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);


	Node* y = x->m_rightChild;

	x->m_rightChild = y->m_leftChild;

	if (y->m_leftChild != m_NIL)
	{
		y->m_leftChild->m_parent = x;
	}

	y->m_parent=x->m_parent;

	if (x == x->m_parent->m_leftChild)
	{
		x->m_parent->m_leftChild = y;
	}
	else
	{
		x->m_parent->m_rightChild = y;
	}

	y->m_leftChild = x;
	x->m_parent = y;

	RAD_ASSERT(m_root == &m_rootStorage);
	RAD_ASSERT(m_NIL == &m_nilStorage);

	RAD_ASSERT_MSG(!m_NIL->m_red,"RedBlackNodeTree::LeftRotate :: sentinel is red!");
	RAD_ASSERT_MSG(!m_root->m_red,"RedBlackNodeTree::LeftRotate :: root is red!");
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::RightRotate()
//////////////////////////////////////////////////////////////////////////////////////////

void RedBlackNodeTree::RightRotate(Node* y)
{
	RAD_ASSERT(y);
	RAD_ASSERT(y->m_magicID == Node::MAGIC_ID);
	RAD_ASSERT(y->m_inTree == this);

	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);


	Node* x = y->m_leftChild;

	y->m_leftChild = x->m_rightChild;

	if (x->m_rightChild != m_NIL)
	{
		x->m_rightChild->m_parent = y;
	}

	x->m_parent = y->m_parent;

	if (y == y->m_parent->m_leftChild)
	{
		y->m_parent->m_leftChild = x;
	}
	else
	{
		y->m_parent->m_rightChild = x;
	}

	x->m_rightChild = y;
	y->m_parent = x;

	RAD_ASSERT(m_root == &m_rootStorage);
	RAD_ASSERT(m_NIL == &m_nilStorage);

	RAD_ASSERT_MSG(!m_NIL->m_red,"RedBlackNodeTree::RightRotate :: sentinel is red!");
	RAD_ASSERT_MSG(!m_root->m_red,"RedBlackNodeTree::RightRotate :: root is red!");
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::PrivateInsert()
//////////////////////////////////////////////////////////////////////////////////////////

void RedBlackNodeTree::PrivateInsert(Node * x)
{
	RAD_ASSERT(x);
	RAD_ASSERT(x->m_magicID == Node::MAGIC_ID);
	RAD_ASSERT(!x->m_inTree);

#if defined (RAD_OPT_DEBUG)
	x->m_inTree = this;
#endif


	x->m_red = true;

	while (x->m_parent->m_red)
	{
		RAD_ASSERT(x->m_parent != m_NIL);
		RAD_ASSERT(x->m_parent->m_parent != m_NIL);

		Node* grand_parent = x->m_parent->m_parent;

		if (x->m_parent == grand_parent->m_leftChild)
		{
			Node* y = grand_parent->m_rightChild;

			if (y->m_red)
			{
				x->m_parent->m_red = y->m_red = false;
				grand_parent->m_red = true;

				x = grand_parent;
			}
			else
			{
				if (x == x->m_parent->m_rightChild)
				{
					x = x->m_parent;

					LeftRotate(x);
				}

				x->m_parent->m_red = false;
				grand_parent->m_red = true;

				RightRotate(grand_parent);
			}
		}
		else
		{
			Node* y = grand_parent->m_leftChild;

			if (y->m_red)
			{
				x->m_parent->m_red = y->m_red = false;
				grand_parent->m_red = true;

				x = grand_parent;
			}
			else
			{
				if (x == x->m_parent->m_leftChild)
				{
					x = x->m_parent;

					RightRotate(x);
				}

				x->m_parent->m_red = false;
				grand_parent->m_red = true;

				LeftRotate(grand_parent);
			}
		}
	}

	m_root->m_leftChild->m_red = false;

	m_size++;

	RAD_ASSERT(m_root == &m_rootStorage);
	RAD_ASSERT(m_NIL == &m_nilStorage);

	RAD_ASSERT_MSG(!m_NIL->m_red,"RedBlackNodeTree::Insert :: sentinel is red!");
	RAD_ASSERT_MSG(!m_root->m_red,"RedBlackNodeTree::Insert :: root is red!");
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::RemoveFixUp()
//////////////////////////////////////////////////////////////////////////////////////////

void RedBlackNodeTree::RemoveFixUp(Node* x)
{
	RAD_ASSERT(x);
	RAD_ASSERT(x->m_magicID == Node::MAGIC_ID);
	RAD_ASSERT(x->m_inTree == this || x == m_NIL);

	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);


	Node * w;
	Node * realRoot = m_root->m_leftChild;

	while (!x->m_red && x != realRoot)
	{
		if (x == x->m_parent->m_leftChild)
		{
			w = x->m_parent->m_rightChild;

			if (w->m_red)
			{
				w->m_red = false;
				x->m_parent->m_red = true;

				LeftRotate(x->m_parent);

				w = x->m_parent->m_rightChild;
			}

			if (!w->m_rightChild->m_red && !w->m_leftChild->m_red)
			{
				w->m_red = true;

				x = x->m_parent;
			}
			else
			{
				if (!w->m_rightChild->m_red)
				{
					w->m_leftChild->m_red = false;
					w->m_red = true;

					RightRotate(w);

					w = x->m_parent->m_rightChild;
				}

				w->m_red = x->m_parent->m_red;
				x->m_parent->m_red = w->m_rightChild->m_red = false;

				LeftRotate(x->m_parent);

				x = realRoot;
			}
		}
		else
		{
			w = x->m_parent->m_leftChild;

			if (w->m_red)
			{
				w->m_red = false;
				x->m_parent->m_red = true;

				RightRotate(x->m_parent);

				w = x->m_parent->m_leftChild;
			}

			if (!w->m_rightChild->m_red && !w->m_leftChild->m_red)
			{
				w->m_red = true;
				x = x->m_parent;
			}
			else
			{
				if (!w->m_leftChild->m_red)
				{
					w->m_rightChild->m_red = false;
					w->m_red = true;

					LeftRotate(w);

					w = x->m_parent->m_leftChild;
				}

				w->m_red = x->m_parent->m_red;
				x->m_parent->m_red = w->m_leftChild->m_red = false;

				RightRotate(x->m_parent);

				x = realRoot;
			}
		}
	}

	x->m_red = false;

	RAD_ASSERT(m_root == &m_rootStorage);
	RAD_ASSERT(m_NIL == &m_nilStorage);

	RAD_ASSERT_MSG(!m_NIL->m_red,"RedBlackNodeTree::RemoveFixUp :: sentinel is red!");
	RAD_ASSERT_MSG(!m_root->m_red,"RedBlackNodeTree::RemoveFixUp :: root is red!");
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::Remove()
//////////////////////////////////////////////////////////////////////////////////////////

void RedBlackNodeTree::Remove(Node * z)
{
	RAD_ASSERT(z);
	RAD_ASSERT(z->m_magicID == Node::MAGIC_ID);
	RAD_ASSERT(z->m_inTree == this);

	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);


	Node* y;
	Node* x;

	if (z->m_leftChild == m_NIL || z->m_rightChild == m_NIL)
	{
		y = z;
	}
	else
	{
		y = Successor(z);
	}

	if (y->m_leftChild == m_NIL)
	{
		x = y->m_rightChild;
	}
	else
	{
		x = y->m_leftChild;
	}


	x->m_parent = y->m_parent;

	if (m_root == x->m_parent)
	{
		m_root->m_leftChild = x;
	}
	else
	{
		if (y == y->m_parent->m_leftChild)
		{
			y->m_parent->m_leftChild = x;
		}
		else
		{
			y->m_parent->m_rightChild = x;
		}
	}

	if (y != z)
	{
		RAD_ASSERT(m_NIL != y);

		y->m_leftChild				= z ->m_leftChild;
		y->m_rightChild				= z->m_rightChild;
		y->m_parent					= z->m_parent;
		z->m_leftChild->m_parent	= y;
		z->m_rightChild->m_parent	= y;

		if (z == z->m_parent->m_leftChild)
		{
			z->m_parent->m_leftChild = y;
		}
		else
		{
			z->m_parent->m_rightChild = y;
		}

		if (!y->m_red)
		{
			y->m_red = z->m_red;

			RemoveFixUp(x);
		}
		else
		{
			y->m_red = z->m_red;
		}
	}
	else
	{
		if (!y->m_red)
		{
			RemoveFixUp(x);
		}
	}

#if defined (RAD_OPT_DEBUG)
	z->m_inTree = NULL;
#endif

	m_size--;


	RAD_ASSERT(m_root == &m_rootStorage);
	RAD_ASSERT(m_NIL == &m_nilStorage);

	RAD_ASSERT_MSG(!m_NIL->m_red,"RedBlackNodeTree::Remove :: sentinel is red!");
	RAD_ASSERT_MSG(!m_root->m_red,"RedBlackNodeTree::Remove :: root is red!");
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::Successor()
//////////////////////////////////////////////////////////////////////////////////////////

RedBlackNodeTree::Node* RedBlackNodeTree::Successor(const Node * x) const
{
	RAD_ASSERT(x);
	RAD_ASSERT(x->m_magicID == Node::MAGIC_ID);
	RAD_ASSERT(x->m_inTree == this);

	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);


	Node* y = x->m_rightChild;

	if (y != m_NIL)
	{
		while (y->m_leftChild != m_NIL)
		{
			y = y->m_leftChild;
		}
	}
	else
	{
		y = x->m_parent;

		while (x == y->m_rightChild)
		{
			x = y;
			y = y->m_parent;
		}

		if (y == m_root)
		{
			return NULL;
		}
	}

	RAD_ASSERT(y != m_NIL);
	RAD_ASSERT(y != m_root);

	RAD_ASSERT(y->m_magicID == Node::MAGIC_ID);
	RAD_ASSERT(y->m_inTree == this);

	return y;
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::Predecessor()
//////////////////////////////////////////////////////////////////////////////////////////

RedBlackNodeTree::Node* RedBlackNodeTree::Predecessor(const Node * x) const
{
	RAD_ASSERT(x);
	RAD_ASSERT(x->m_magicID == Node::MAGIC_ID);
	RAD_ASSERT(x->m_inTree == this);

	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);


	Node* y = x->m_leftChild;

	if (y != m_NIL)
	{
		while (y->m_rightChild != m_NIL)
		{
			y = y->m_rightChild;
		}
	}
	else
	{
		y = x->m_parent;

		while (x == y->m_leftChild)
		{
			if (y == m_root)
			{
				return NULL;
			}

			x = y;
			y = y->m_parent;
		}
	}

	RAD_ASSERT(y != m_NIL);
	RAD_ASSERT(y != m_root);

	RAD_ASSERT(y->m_magicID == Node::MAGIC_ID);
	RAD_ASSERT(y->m_inTree == this);

	return y;
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::Traverse()
//////////////////////////////////////////////////////////////////////////////////////////

void RedBlackNodeTree::Traverse(TraversalType type,TraverseProc callback,void* userData) const
{
	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);

	Node* n = m_root->m_leftChild;

	if (n != m_NIL)
	{
		switch (type)
		{
		case PRE_ORDER:		PrivateTraversePreorder(callback,n,userData);		break;
		case IN_ORDER:		PrivateTraverseInorder(callback,n,userData);		break;
		case POST_ORDER:	PrivateTraversePostorder(callback,n,userData);		break;
		default:			RAD_ASSERT(NULL);									break;
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::PrivateTraversePreorder()
//////////////////////////////////////////////////////////////////////////////////////////

void RedBlackNodeTree::PrivateTraversePreorder(TraverseProc callback,Node* n,void* userData) const
{
	RAD_ASSERT(n);
	RAD_ASSERT(n->m_magicID == Node::MAGIC_ID);
	RAD_ASSERT(n->m_inTree == this);

	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);


	(*callback)(this,n,userData);

	Node* left = n->LeftChild();
	Node* right = n->RightChild();

	if (left != m_NIL)
		PrivateTraversePreorder(callback,left,userData);

	if (right != m_NIL)
		PrivateTraversePreorder(callback,right,userData);
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::PrivateTraverseInorder()
//////////////////////////////////////////////////////////////////////////////////////////

void RedBlackNodeTree::PrivateTraverseInorder(TraverseProc callback,Node* n,void* userData) const
{
	RAD_ASSERT(n);
	RAD_ASSERT(n->m_magicID == Node::MAGIC_ID);
	RAD_ASSERT(n->m_inTree == this);

	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);


	Node* left = n->LeftChild();
	Node* right = n->RightChild();

	if (left != m_NIL)
		PrivateTraverseInorder(callback,left,userData);

	(*callback)(this,n,userData);

	if (right != m_NIL)
		PrivateTraverseInorder(callback,right,userData);
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::PrivateTraversePostorder()
//////////////////////////////////////////////////////////////////////////////////////////

void RedBlackNodeTree::PrivateTraversePostorder(TraverseProc callback,Node* n,void* userData) const
{
	RAD_ASSERT(n);
	RAD_ASSERT(n->m_magicID == Node::MAGIC_ID);
	RAD_ASSERT(n->m_inTree == this);

	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);


	Node* left = n->LeftChild();
	Node* right = n->RightChild();

	if (left != m_NIL)
		PrivateTraversePostorder(callback,left,userData);

	if (right != m_NIL)
		PrivateTraversePostorder(callback,right,userData);

	(*callback)(this,n,userData);
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::First()
//////////////////////////////////////////////////////////////////////////////////////////

RedBlackNodeTree::Node* RedBlackNodeTree::First() const
{
	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);

	Node* node = m_root->m_leftChild;

	if (node != m_NIL)
	{
		while (node->m_leftChild != m_NIL)
		{
			node = node->m_leftChild;
		}

		RAD_ASSERT(node->m_magicID == Node::MAGIC_ID);
		RAD_ASSERT(node->m_inTree == this);

		return node;
	}

	return NULL;
}


//////////////////////////////////////////////////////////////////////////////////////////
// RedBlackNodeTree::Last()
//////////////////////////////////////////////////////////////////////////////////////////

RedBlackNodeTree::Node* RedBlackNodeTree::Last() const
{
	RAD_ASSERT_MSG(m_magicID == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);

	Node* node = m_root->m_leftChild;

	if (node != m_NIL)
	{
		while (node->m_rightChild != m_NIL)
		{
			node = node->m_rightChild;
		}

		RAD_ASSERT(node->m_magicID == Node::MAGIC_ID);
		RAD_ASSERT(node->m_inTree == this);

		return node;
	}

	return NULL;
}

} // container

