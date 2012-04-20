// RedBlackNodeTree.h
// A RedBlack Tree.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntContainer.h"
#include "ContainerCommon.h"
#include "../PushPack.h"


namespace container {

//////////////////////////////////////////////////////////////////////////////////////////
//
// container::RedBlackNodeTree
//
//////////////////////////////////////////////////////////////////////////////////////////
//
// A lightweight red black tree that requires the user to derrive from a base node
// class.
//
// Important NOTE: RedBlackNodeTree does not make a copy of objects that are inserted! This
// is for performance purposes.
//
//////////////////////////////////////////////////////////////////////////////////////////


class RADRT_CLASS RedBlackNodeTree
{
public:

	class RADRT_CLASS Node
	{
	public:

		Node();
		virtual ~Node();

		Node* LeftChild() const;
		Node* RightChild() const;

		Node* Parent() const;


	private:

#if defined (RAD_OPT_DEBUG)
		enum { MAGIC_ID = RAD_FOURCC_LE('R','B','T','N') };

		AddrSize m_magicID;
		RedBlackNodeTree* m_inTree;
#endif

		Node* m_leftChild;
		Node* m_rightChild;
		Node* m_parent;
		bool  m_red;

		// == 20 bytes release 32 bit target, 36 on a 64 bit target (including vtable).
		// == 28 bytes debug 32 bit target, 52 on a 64 bit target (including vtable).

		friend class RedBlackNodeTree;
	};

	//
	// Constructor / Destructor
	//

	RedBlackNodeTree();
	virtual ~RedBlackNodeTree();

	//
	// Insert / Removal
	//

	//
	// SReg compare(_ARG0_ADAPTER(Node*), _ARG1_ADAPTER(Node*))
	//
	template<typename _COMPARE_PREDICATE>
	void Insert(Node* n,_COMPARE_PREDICATE& compare);
	template<typename _COMPARE_PREDICATE>
	void Resort(Node* n,_COMPARE_PREDICATE& compare);
	void Remove(Node* n);

	//
	// Find.
	//
	// SReg compare(_ARG0_ADAPTER(const _SORT_VALUE&), _ARG1_ADAPTER(const Node*))
	//

	template <typename _SORT_VALUE,class _COMPARE_PREDICATE>
	Node* Find(const _SORT_VALUE& sortValue,_COMPARE_PREDICATE& compare) const;

	//
	// Traversal / Iteration
	//

	typedef void (*TraverseProc)(const RedBlackNodeTree* tree,const Node* n,void* userData);

	void Traverse(TraversalType type,TraverseProc callback,void* userData) const;

	Node* Root() const;
	const Node* NIL() const;

	Node* Predecessor(const Node* n) const;
	Node* Successor(const Node* n) const;

	Node* First() const;
	Node* Last() const;

	Node* Next(const Node* n) const;
	Node* Previous(const Node* n) const;


	//
	// Utilities
	//

	void Clear();

	bool IsEmpty() const;
	UReg Size() const;


protected:

	RedBlackNodeTree(const RedBlackNodeTree&);
	RedBlackNodeTree& operator = (const RedBlackNodeTree&);

	void PrivateInsert(Node* n);

	template<typename _COMPARE_PREDICATE>
	void InsertFixUp(Node* n,_COMPARE_PREDICATE& compare);
	void RemoveFixUp(Node* n);

	void LeftRotate(Node* n);
	void RightRotate(Node* n);

	void PrivateTraversePreorder(TraverseProc callback,Node* n,void* userData) const;
	void PrivateTraverseInorder(TraverseProc callback,Node* n,void* userData) const;
	void PrivateTraversePostorder(TraverseProc callback,Node* n,void* userData) const;


#if defined (RAD_OPT_DEBUG)
	enum { MAGIC_ID = RAD_FOURCC_LE('R','B','N','T') };
	AddrSize m_magicID;
#endif
	Node  m_rootStorage;
	Node  m_nilStorage;
	Node* m_root;
	Node* m_NIL;
	UReg  m_size;
};

} // container


#include "../PopPack.h"
#include "RedBlackNodeTree.inl"
