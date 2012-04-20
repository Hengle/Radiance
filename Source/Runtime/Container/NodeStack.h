// NodeStack.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "NodeList.h"
#include "../PushPack.h"


namespace container {

class RADRT_CLASS NodeStack : public NodeList
{
public:

	NodeStack();
	virtual ~NodeStack();

	void			Push(NodeList::Node* n);
	NodeList::Node* Pop();

private:

	NodeStack(const NodeStack& s);
	NodeStack& operator = (const NodeStack& s);
};

} // container


#include "../PopPack.h"
#include "NodeStack.inl"
