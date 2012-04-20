// NodeQueue.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "NodeList.h"
#include "../PushPack.h"


namespace container {

class RADRT_CLASS NodeQueue : public NodeList
{
public:

	NodeQueue();
	virtual ~NodeQueue();

	void			Push(NodeList::Node* n);
	NodeList::Node* Pop();

private:

	NodeQueue(const NodeQueue& q);
	NodeQueue& operator = (const NodeQueue& q);
};

} // container


#include "../PopPack.h"
#include "NodeQueue.inl"
