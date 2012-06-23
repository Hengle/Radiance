// Reflect.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy & Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Reflect.h"
#include "../StringBase.h"
#include "../Predicate.h"
#include "../Stream.h"

#undef max
#undef min

namespace reflect {

const char *NamedFunction::InvalidArgumentDescription() const
{
	return "reflect::NamedFunction: Invalid argument";
}

const char *NamedFunction::MissingArgumentDescription() const
{
	return "reflect::NamedFunction: Missing argument";
}

const char *AnonymousFunction::InvalidArgumentDescription() const
{
	return "reflect::AnonymousFunction: Invalid argument";
}

const char *AnonymousFunction::MissingArgumentDescription() const
{
	return "reflect::AnonymousFunction: Missing argument";
}

void Function::ThrowInvalidArgumentException(int argument) const
{
	throw InvalidArgumentExceptionType(this, argument);
}

void Function::ThrowMissingArgumentException(int argument) const
{
	throw MissingArgumentExceptionType(this, argument);
}

Class::Class(
	const ATTRIBUTEARRAY      &attributes,
	const NAME                &name,
	AddrSize                  size,
	AddrSize                  alignment,
	const SUPERARRAY          &supers,
	const MEMBERARRAY         &members,
	const STATICMEMBERARRAY   &staticMembers,
	const STATICCONSTANTARRAY &staticConstants,
	const CONSTRUCTORARRAY    &constructors,
	const MUTABLEMETHODARRAY  &methods,
	const CONSTMETHODARRAY    &constMethods,
	const STATICMETHODARRAY   &staticMethods,
	DTOR                      dtor
) :
Named(attributes, name),
m_supers(supers),
m_members(members),
m_staticMembers(staticMembers),
m_staticConstants(staticConstants),
m_constructors(constructors),
m_methods(methods),
m_constMethods(constMethods),
m_staticMethods(staticMethods),
m_size(size),
m_dtor(dtor),
m_alignment((U8)alignment)
{
	RAD_ASSERT(alignment <= std::numeric_limits<U8>::max());
}

Class::~Class()
{
}

bool Class::IsA(const Class *type) const
{
	if (NULL == type) { return false; }

	const Class *t = this;
	if (type == t) { return true; }

	const int NUM_SUPERS = NumSupers();
	for (int i = 0; i < NUM_SUPERS; i++)
	{
		const Class::SUPER *s = Super(i);
		RAD_ASSERT(NULL != s);
		t = s->Type();
		RAD_ASSERT(NULL != t);
		if (t->IsA(type)) { return true; }
	}

	return false;
}

void Class::Constructor::ThrowInvalidArgumentException(int argument) const
{
	throw InvalidArgumentExceptionType(this, argument);
}

void Class::Constructor::ThrowMissingArgumentException(int argument) const
{
	throw MissingArgumentExceptionType(this, argument);
}

const Class::CONSTRUCTOR *Class::FindConstructor(const ITypeList &args) const
{
	const int NUM_METHODS = NumConstructors();
	for (int i = 0; i < NUM_METHODS; ++i)
	{
		const CONSTRUCTOR *method = Constructor(i);
		if (method->Match(args)) { return method; }
	}
	return NULL;
}

void Class::ConstMethod::ThrowInvalidArgumentException(int argument) const
{
	throw InvalidArgumentExceptionType(this, argument);
}

void Class::ConstMethod::ThrowMissingArgumentException(int argument) const
{
	throw MissingArgumentExceptionType(this, argument);
}

void Class::MutableMethod::ThrowInvalidArgumentException(int argument) const
{
	throw InvalidArgumentExceptionType(this, argument);
}

void Class::MutableMethod::ThrowMissingArgumentException(int argument) const
{
	throw MissingArgumentExceptionType(this, argument);
}

void Class::StaticMethod::ThrowInvalidArgumentException(int argument) const
{
	throw InvalidArgumentExceptionType(this, argument);
}

void Class::StaticMethod::ThrowMissingArgumentException(int argument) const
{
	throw MissingArgumentExceptionType(this, argument);
}

ConstReflected ConstReflected::Super(const Class::SUPER *super) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(NULL != super);

	const int NUM_SUPERS = NumSupers();
	for (int i = 0; i < NUM_SUPERS; ++i)
	{
		const Class::SUPER *s = m_type->Super(i);
		if (s == super) { return UncheckedCastByType(s); }
		ConstReflected r = Super(i).Super(super);
		if (r) { return r; }
	}

	return ConstReflected();
}

ConstReflected ConstReflected::CastByType(const Class *type) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(NULL != type);

	const int NUM_SUPERS = NumSupers();
	for (int i = 0; i < NUM_SUPERS; ++i)
	{
		ConstReflected super = Super(i);
		if (super.Type() == type) { return super;	}
		ConstReflected r = super.CastByType(type);
		if (r) { return r; }
	}

	return ConstReflected();
}

template <typename T>
inline bool CopyByVal(
	Reflected &out,
	Zone &zone,
	const ConstReflected &r
)
{
	if (r.Type() == Type<T>())
	{
		out = Reflect(new (SafeAllocate(zone, Type<T>())) T(*static_cast<const T*>(r)), r.Type());
		RAD_ASSERT(out.IsValid());
		return true;
	}
	return false;
}

Reflected Reflected::Clone(
	Zone &zone,
	const ConstReflected &r
)
{
	RAD_ASSERT(r.IsValid());
	Reflected x;

#define C(_x) CopyByVal<_x>(x, zone, r)

	bool byVal = C(S32)||C(S32*)||C(U32)||C(U32*)||C(void*)||C(bool)||C(bool*)||
		C(char)||C(char*)||C(wchar_t)||C(wchar_t*)||C(S64)||C(S64*)||C(U64)||C(U64*)||
		C(F32)||C(F32*)||C(F64)||C(F64*)||C(S16)||C(S16*)||C(U16)||C(U16*)||
		C(string::String)||C(std::string)||C(std::wstring);

	if (!byVal)
	{
		x = Reflected::New(
			zone,
			r.Type(),
			FixedArgumentList<1>().PushBack(r)
		);
	}

	return x;
}

} // reflect

