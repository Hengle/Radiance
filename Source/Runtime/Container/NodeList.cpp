// NodeList.cpp
// Inherited list.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "IntContainer.h"
#include "NodeList.h"


namespace container {

#if defined(RAD_OPT_DEBUG)
static const char* STDPTR_DEBUG_ERROR_BADMEMORY = "Bad Pointer / Memory Corruption Detected!";
#endif
	
//////////////////////////////////////////////////////////////////////////////////////////
// containter::NodeList::NodeList()
//////////////////////////////////////////////////////////////////////////////////////////

NodeList::NodeList(const NodeList& list)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// containter::NodeList::operator = ()
//////////////////////////////////////////////////////////////////////////////////////////

NodeList& NodeList::operator = (const NodeList& list)
{
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////

//
// Debug
//

//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// containter::NodeList::AssertValid()
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_DEBUG)
void NodeList::AssertValid()
{
	RAD_ASSERT_MSG(m_id == MAGIC_ID, STDPTR_DEBUG_ERROR_BADMEMORY);
	if (m_head)
	{
		m_head->AssertValid();
		RAD_ASSERT(m_head->m_prev == 0);
	}
	if (m_tail)
	{
		m_tail->AssertValid();
		RAD_ASSERT(m_tail->m_next == 0);
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// containter::NodeList::InsertAfter()
//////////////////////////////////////////////////////////////////////////////////////////

void NodeList::InsertAfter(Node* after, Node* node)
{
	RAD_ASSERT(node);

#if defined(RAD_OPT_DEBUG)
	AssertValid();
	node->AssertValid();

	if (after)
	{
		after->AssertValid();
		RAD_ASSERT_MSG(after->m_list == this, "Attempting to insert after a node that is not in this list");
	}

#endif

	RAD_ASSERT_MSG(node->m_list == 0, "Node is already in a list!");
	RAD_ASSERT(node->m_next == 0 && node->m_prev == 0);

	node->m_prev = after;

	if (after)
	{
		node->m_next = after->m_next;
		if (after->m_next) after->m_next->m_prev = node;
		after->m_next = node;
	}
	else // insert at head.
	{
		node->m_next = m_head;
		if (m_head)
		{
			RAD_ASSERT(m_head->m_prev == 0);
			m_head->m_prev = node;
			m_head = node;
		}
	}

	if (m_head == 0)
	{
		RAD_ASSERT(m_tail == 0);

		m_head = m_tail = node;
	}

	if (after == m_tail)
	{
		m_tail = node;
	}

#if defined(RAD_OPT_DEBUG)
	node->m_list = this;
#endif

	m_size++;
}

//////////////////////////////////////////////////////////////////////////////////////////

//
// Insertion/Removal functions.
//

//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// containter::NodeList::InsertBefore()
//////////////////////////////////////////////////////////////////////////////////////////

void NodeList::InsertBefore(Node* before, Node* node)
{
	RAD_ASSERT(node);

#if defined(RAD_OPT_DEBUG)
	AssertValid();
	node->AssertValid();

	if (before)
	{
		before->AssertValid();
		RAD_ASSERT_MSG(before->m_list == this, "Attempting to insert before a node that is not in this list");
	}

#endif

	RAD_ASSERT_MSG(node->m_list == 0, "Node is already in a list!");
	RAD_ASSERT(node->m_next == 0 && node->m_prev == 0);

	node->m_next = before;

	if (before)
	{
		node->m_prev = before->m_prev;
		if (before->m_prev) before->m_prev->m_next = node;
		before->m_prev = node;
	}
	else // insert at head.
	{
		node->m_next = m_head;
		if (m_head)
		{
			RAD_ASSERT(m_head->m_prev == 0);
			m_head->m_prev = node;
			m_head = node;
		}
	}

	if (m_head == 0)
	{
		RAD_ASSERT(m_tail == 0);

		m_head = m_tail = node;
	}

	if (before == m_head)
	{
		m_head = node;
	}

#if defined(RAD_OPT_DEBUG)
	node->m_list = this;
#endif

	m_size++;
}

//////////////////////////////////////////////////////////////////////////////////////////
// containter::NodeList::Remove()
//////////////////////////////////////////////////////////////////////////////////////////

void NodeList::Remove(Node* node)
{
	RAD_ASSERT(m_size > 0);
	RAD_ASSERT(node);

#if defined(RAD_OPT_DEBUG)
	AssertValid();
	node->AssertValid();

	if (m_head)
	{
		RAD_ASSERT(m_head->m_prev == 0);
	}
	if (m_tail)
	{
		RAD_ASSERT(m_tail->m_next == 0);
	}

	RAD_ASSERT_MSG(node->m_list == this, "Node not owned by this list!");
#endif

	if (node->m_prev) node->m_prev->m_next = node->m_next;
	if (node->m_next) node->m_next->m_prev = node->m_prev;

	if (node == m_head)
	{
		RAD_ASSERT(node->m_prev == 0);
		m_head = node->m_next;
	}

	if (node == m_tail)
	{
		RAD_ASSERT(node->m_next == 0);
		m_tail = node->m_prev;
	}

#if defined(RAD_OPT_DEBUG)
	node->m_prev = node->m_next = 0;
	node->m_list = 0;
#endif

	--m_size;
}

} // container

