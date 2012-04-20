// LWNodeList.cpp
// Inherited list (with no vtable).
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "IntContainer.h"
#include "LWNodeList.h"


namespace container {

#if defined(RAD_OPT_DEBUG)

static const char* STDPTR_DEBUG_ERROR_BADMEMORY = "Bad Pointer / Memory Corruption Detected!";

#endif

//////////////////////////////////////////////////////////////////////////////////////////
// containter::LWNodeList::LWNodeList()
//////////////////////////////////////////////////////////////////////////////////////////

LWNodeList::LWNodeList(const LWNodeList& list)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// containter::LWNodeList::operator = ()
//////////////////////////////////////////////////////////////////////////////////////////

LWNodeList& LWNodeList::operator = (const LWNodeList& list)
{
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////

//
// Debug
//

//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// containter::LWNodeList::AssertValid()
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_DEBUG)
void LWNodeList::AssertValid()
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
// containter::LWNodeList::InsertAfter()
//////////////////////////////////////////////////////////////////////////////////////////

void LWNodeList::InsertAfter(Node* after, Node* node)
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
#if defined(RAD_OPT_DEBUG)
		RAD_ASSERT(m_tail == 0);
#endif
		m_head = m_tail = node;
	}

	if (after == m_tail) m_tail = node;

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
// containter::LWNodeList::InsertBefore()
//////////////////////////////////////////////////////////////////////////////////////////

void LWNodeList::InsertBefore(Node* before, Node* node)
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
#if defined(RAD_OPT_DEBUG)
		RAD_ASSERT(m_tail == 0);
#endif
		m_head = m_tail = node;
	}

	if (before == m_head) m_head = node;

#if defined(RAD_OPT_DEBUG)
	node->m_list = this;
#endif

	m_size++;
}

//////////////////////////////////////////////////////////////////////////////////////////
// containter::LWNodeList::Remove()
//////////////////////////////////////////////////////////////////////////////////////////

void LWNodeList::Remove(Node* node)
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

	m_size--;
}

} // container

