// LWNodeList.h
// Inherited list (with no vtable).
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntContainer.h"
#include "Iterator.h"
#include "../PushPack.h"


namespace container {

//////////////////////////////////////////////////////////////////////////////////////////
//
// container::LWNodeList
//
//////////////////////////////////////////////////////////////////////////////////////////
//
// A lightweight doubly linked list that requires the user to derrive from a base node
// class.
//
// Important NOTE: LWNodeList does not make a copy of objects that are inserted! This is for
// performance purposes.
//
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS LWNodeList
{
public:
	LWNodeList();
	virtual ~LWNodeList();

	class BaseIterator;

	//
	// Users of the LWNodeList class must derrive from this class to place their objects
	// in the list.
	//
	class RADRT_CLASS Node
	{
	public:
		Node();
		Node(const Node& n);
		~Node();

		Node* Next();
		const Node* Next() const;

		Node* Prev();
		const Node* Prev() const;


	private:

#if defined(RAD_OPT_DEBUG)
		enum { MAGIC_ID = RAD_FOURCC_LE('N', 'L', 'N', 'D') };
		AddrSize m_id;
		LWNodeList* m_list;
		void AssertValid() const;
#endif

		Node* m_next, *m_prev;

		// == 8 bytes release 32 bit target, 16 on a 64 bit target (including vtable).
		// == 16 bytes debug 32 bit target, 32 on a 64 bit target (including vtable).

		friend class LWNodeList;
		friend class LWNodeList::BaseIterator;
	};

	//
	// Insertion functions.
	//

	void InsertFirst(Node* node);
	void InsertLast (Node* node);

	void InsertAfter (Node* after,  Node* nodeToInsert);
	void InsertBefore(Node* before, Node* nodeToInsert);

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
	friend class LWNodeList;

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
		LWNodeList* m_list;
	};

	typedef iterator::BDSimpleIterator < BaseIterator, Node* > Iterator;

	Iterator Begin() const;
	Iterator BeginAt(const Node* node) const;
	Iterator End() const;

private:

	LWNodeList(const LWNodeList& list);
	LWNodeList& operator = (const LWNodeList& list);

#if defined(RAD_OPT_DEBUG)
	enum { MAGIC_ID = RAD_FOURCC_LE('T', 'S', 'L', 'N') };
	AddrSize m_id;
	void AssertValid();
#endif

	UReg  m_size;
	Node* m_head;
	Node* m_tail;
};

} // container


#include "../PopPack.h"
#include "LWNodeList.inl"
