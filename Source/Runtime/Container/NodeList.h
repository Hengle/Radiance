// NodeList.h
// Inherited list.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel

#pragma once

#include "IntContainer.h"
#include "Iterator.h"
#include "../PushPack.h"


namespace container {

//////////////////////////////////////////////////////////////////////////////////////////
//
// container::NodeList
//
//////////////////////////////////////////////////////////////////////////////////////////
//
// A lightweight doubly linked list that requires the user to derive from a base node
// class.
//
// Important NOTE: NodeList does not make a copy of objects that are inserted! This is for
// performance purposes.
//
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS NodeList
{
public:
	NodeList();
	virtual ~NodeList();

	class BaseIterator;

	//
	// Users of the NodeList class must derrive from this class to place their objects
	// in the list.
	//
	class RADRT_CLASS Node
	{
	public:
		Node();
		Node(const Node& n);
		virtual ~Node();

		Node* Next();
		const Node* Next() const;

		Node* Prev();
		const Node* Prev() const;


	private:

#if defined(RAD_OPT_DEBUG)
		enum { MAGIC_ID = RAD_FOURCC_LE('N', 'L', 'N', 'D') };
		AddrSize m_id;
		NodeList* m_list;
		void AssertValid() const;
#endif

		Node* m_next, *m_prev;

		// == 12 bytes release 32 bit target, 24 on a 64 bit target (including vtable).
		// == 20 bytes debug 32 bit target, 40 on a 64 bit target (including vtable).

		friend class NodeList;
		friend class NodeList::BaseIterator;
	};

	//
	// Insertion functions.
	//

	void InsertFirst(Node* node);
	void InsertLast (Node* node);

	void InsertAfter (Node* nodeToInsert, Node* after);
	void InsertBefore(Node* nodeToInsert, Node* before);

	//
	// Removal.
	//

	void Remove(Node* node);

	Node* RemoveFirst();
	Node* RemoveLast();


	//
	// First/Last
	//

	Node* First();
	const Node* First() const;

	Node* Last();
	const Node* Last() const;

	//
	// Utilization
	//

	void Clear();
	UReg Size() const;
	bool IsEmpty() const;

	//
	// Sorting
	//
	// SReg compare(_ARG0_ADAPTER(const Node*), _ARG1_ADAPTER(const Node*))
	//

	template< typename _COMPARE_PREDICATE >
	void InsertionSortInsert(Node* node, _COMPARE_PREDICATE& compare); // <-- only works if the list is already sorted.
	template< typename _COMPARE_PREDICATE >
	void InsertionSortResort(Node* node, _COMPARE_PREDICATE& compare); // <-- only works if the list is already sorted.
	template< typename _COMPARE_PREDICATE >
	void MergeSort(_COMPARE_PREDICATE& compare); // Sorts the entire list.

	//
	// Iteration
	//

	class RADRT_CLASS BaseIterator
	{
	friend class iterator::SimpleIterator  < BaseIterator, Node* >;
	friend class iterator::BDSimpleIterator< BaseIterator, Node* >;
	friend class NodeList;

	private:
		BaseIterator();
		~BaseIterator();

		void Next();
		void Prev();

		Node* const& Value() const;

		bool IsEqual(const BaseIterator& iterator) const;
		bool IsGreater(const BaseIterator& iterator) const;
		bool IsLess(const BaseIterator& iterator) const;
		bool IsValid() const;

		Node* m_node;
		UReg m_ofs;
		NodeList* m_list;
	};

	typedef iterator::BDSimpleIterator < BaseIterator, Node* > Iterator;

	Iterator Begin() const;
	Iterator BeginAt(const Node* node) const;
	Iterator End() const;

private:

	NodeList(const NodeList& list);
	NodeList& operator = (const NodeList& list);

#if defined(RAD_OPT_DEBUG)
	enum { MAGIC_ID = RAD_FOURCC_LE('N', 'L', 'S', 'T') };
	AddrSize m_id;
	void AssertValid();
#endif

	UReg  m_size;
	Node* m_head;
	Node* m_tail;
};

} // container


#include "../PopPack.h"
#include "NodeList.inl"
