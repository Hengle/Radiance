// NodeListDef.h
// Inherited list.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntContainer.h"


namespace container {

//////////////////////////////////////////////////////////////////////////////////////////
//
// containter::NodeList
//
//////////////////////////////////////////////////////////////////////////////////////////
//
// A lightweight doubly linked list that requires the user to derrive from a base node
// class.
//
// Important NOTE: NodeList does not make a copy of objects that are inserted! This is for
// performance purposes.
//
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS NodeList;

} // container

