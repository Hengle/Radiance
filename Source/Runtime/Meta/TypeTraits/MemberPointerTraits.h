// MemberPointerTraits.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../IntegralConstant.h"


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsMemberPointer<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct IsMemberPointer :
public FalseType
{
};

template <typename T, typename C>
struct IsMemberPointer<T C::*> :
public TrueType
{
};

template <typename T, typename C>
struct IsMemberPointer<T C::*&> :
public TrueType
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::RemoveMemberPointer<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct RemoveMemberPointer
{
	typedef T Type;
};

template <typename T, typename C>
struct RemoveMemberPointer<T C::*>
{
	typedef T Type;
};

template <typename T, typename C>
struct RemoveMemberPointer<T C::*&>
{
	typedef T &Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::AddMemberPointer<T, C>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename C>
struct AddMemberPointer
{
	typedef T C::*Type;
};

template <typename T, typename C>
struct AddMemberPointer<T C::*, C>
{
	typedef T C::*Type;
};

template <typename T, typename C>
struct AddMemberPointer<T C::*&, C>
{
	typedef T C::*&Type;
};

} // meta

