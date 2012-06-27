// EnumFlags.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

//! This class makes typesafe bitflags from enum types.
/*! Use the RAD_BEGIN_FLAGS(), RAD_FLAG(), RAD_END_FLAGS(), and RAD_IMPLEMENT_FLAGS() to create flags from
	your enum types.

	\par Example
	\code
	RAD_BEGIN_FLAGS
		RAD_FLAG(kSystemOption_OPtion1),
		RAD_FLAG(kSystemOption_Option2)
	RAD_END_FLAGS(SystemOptions)

	RAD_IMPLEMENT_FLAGS(SystemOptions)
	\endcode

	The enum flags can be used like so:

	\code
	void methodThatTakesFlags(SystemOptions options);

	void main() {
		methodThatTakesFlags(kSystemOption_Option1|kSystemOption_Option2);
	}
	\endcode

	\note

	The RAD_BEGIN_FLAGS / RAD_END_FLAGS(Name) block declares the raw enum type as kNameOfEnum.
	In the above example the enum type for SystemOptions is kSystemOptions. This type can also
	be referenced as the Enum member of SystemOptions (i.e. SystemOptions::Enum).
*/
template <typename T>
class RadEnumFlags {
public:

	typedef T Enum;
	typedef RadEnumFlags<T> SelfType;
	typedef void (SelfType::*unspecified_bool_type) ();

	RadEnumFlags() : m_i(0) {}
	RadEnumFlags(const SelfType &t) : m_i(t.m_i) {}
	RadEnumFlags(T t) : m_i(t) {}

	operator int () const { return m_i; }

	SelfType &operator = (const SelfType &m) { 
		m_i = m.m_i;
		return *this;
	}

	SelfType &operator &= (const SelfType &m) {
		m_i &= m.m_i;
		return *this;
	}

	SelfType &operator &= (Enum m) {
		m_i &= m;
		return *this;
	}

	SelfType &operator |= (const SelfType &m) {
		m_i |= m.m_i;
		return *this;
	}

	SelfType &operator |= (Enum m) {
		m_i |= m;
		return *this;
	}

	SelfType &operator ^= (const SelfType &m) {
		m_i ^= m.m_i;
		return *this;
	}

	SelfType &operator ^= (Enum m) {
		m_i ^= m;
		return *this;
	}

	SelfType operator | (const SelfType &m) const {
		return SelfType(Enum(m_i|m.m_i));
	}

	SelfType operator | (Enum m) const {
		return SelfType(Enum(m_i|m));
	}

	SelfType operator ^ (const SelfType &m) const {
		return SelfType(Enum(m_i^m.m_i));
	}

	SelfType operator ^ (Enum m) const {
		return SelfType(Enum(m_i^m));
	}

	SelfType operator & (const SelfType &m) const {
		return SelfType(Enum(m_i&m.m_i));
	}

	SelfType operator & (Enum m) const {
		return SelfType(Enum(m_i&m));
	}

	SelfType operator ~ () const {
		return SelfType(Enum(~m_i));
	}

	unspecified_bool_type operator ! () const {
		return (!m_i) ? &bool_true : 0;
	}

private:

	void bool_true() {}

	int m_i;

};

#if defined(_DOXYGEN)

#define RAD_FLAG_BIT(_name, _bit) _name
#define RAD_FLAG(name) _name
#define RAD_BEGIN_FLAGS typedef enum {
#define RAD_END_FLAGS(_type) } _type;
#define RAD_IMPLEMENT_FLAGS(_type)

#else

#define RAD_MKFLAG(_x) (1 << (_x))
#define RAD_FLAG(_name) rad_flag_head_##_name, _name = RAD_MKFLAG(rad_flag_head_##_name), rad_flag_tail_##_name = rad_flag_head_##_name
#define RAD_FLAG_BIT(_name, _bit) _name = RAD_MKFLAG(_bit), rad_flag_tail_##_name = _bit

#define RAD_BEGIN_FLAGS typedef enum {
#define RAD_END_FLAGS(_type) } k##_type; typedef ::RadEnumFlags<k##_type> _type;
#define RAD_IMPLEMENT_FLAGS(_type) \
inline TestFlags operator | (_type::Enum a, _type::Enum b) { \
	return _type(a) | b; \
} \
inline _type operator & (_type::Enum a, _type::Enum b) { \
	return _type(a) & b; \
}


#endif
