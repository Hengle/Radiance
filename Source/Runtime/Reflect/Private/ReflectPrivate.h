// ReflectPrivate.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ReflectPrivateDef.h"
#include "../../PushPack.h"


namespace reflect {

class Class;

//////////////////////////////////////////////////////////////////////////////////////////
// Begin namespace reflect::details
//////////////////////////////////////////////////////////////////////////////////////////

namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassList
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS ClassList
{
public://private:

	static void Insert(Class &type);
	static void Remove(Class &type);

	static const Class *First();
	static const Class *Next(const Class *pos);
	static const Class *Prev(const Class *pos);
	static const Class *Last();

	static const Class *Find(const char *name);
	static const Class *Find(const wchar_t *name);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::FindAttribute()
//////////////////////////////////////////////////////////////////////////////////////////
// Attribute Finding Helpers

template <typename T, typename TChar>
const ATTRIBUTE *FindAttribute(const T *object, const TChar *name);

template<typename T>
const ATTRIBUTE *FindAttribute(const T *object, const Class *type);

template<typename T, typename TChar>
const ATTRIBUTE *FindAttribute(const T *object, const Class *type, const TChar *name);

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::AttributeValue()
//////////////////////////////////////////////////////////////////////////////////////////
// Attribute Value Helpers

template<typename T>
const ATTRIBUTE *AttributeValue(const T *object, const Class *type, ConstReflected *value);

template<typename T, typename TChar>
const ATTRIBUTE *AttributeValue(const T *object, const TChar *name, ConstReflected *value);

template<typename T, typename TChar>
const ATTRIBUTE *AttributeValue(const T *object, const Class *type, const TChar *name, ConstReflected *value);

template<typename T, typename X>
const ATTRIBUTE *AttributeValue(const T *object, X *value);

template<typename T, typename X, typename TChar>
const ATTRIBUTE *AttributeValue(const T *object, const TChar *name, X *value);

} // details
} // reflect


#include "../../PopPack.h"
#include "ReflectPrivate.inl"
