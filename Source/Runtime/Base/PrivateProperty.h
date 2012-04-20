// PrivateProperty.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#define RAD_PRIVATE_PROPERTY_GET_METHOD_NAME(name) __get_##name
#define RAD_PRIVATE_PROPERTY_SET_METHOD_NAME(name) __set_##name
#define RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) __property_##name

//////////////////////////////////////////////////////////////////////////////////////////
// PropertyBase<_radPropT, SUBTYPE>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename _radPropT, typename GetType>
struct PropertyBaseGet
{
	typedef _radPropT TYPE;
	GetType get() const
	{
		return static_cast<const _radPropT*>(this)->get();
	}
};

template <typename _radPropT, typename SetType>
struct PropertyBaseSet
{
	typedef _radPropT TYPE;
	void set(SetType x)
	{
		static_cast<_radPropT*>(this)->set(x);
	}
};

template <typename _radPropT, typename GetType, typename SetType>
struct PropertyBaseGetSet : public PropertyBaseGet<_radPropT, GetType>, public PropertyBaseSet<_radPropT, SetType>
{
	typedef _radPropT TYPE;
};



#define RAD_PRIVATE_PROPERTY_DECLARE_GETSET_CLASS(_ctype, _class, name, get_type, set_type) \
	class RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) : public \
	::PropertyBaseGetSet<RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name), get_type, set_type> { \
		public: \
			template <typename _radPropT, typename _radPropX>\
			bool operator == (const ::PropertyBaseGet<_radPropT, _radPropX> &other)\
			{ return get() == other.get(); }\
			template <typename _radPropT, typename _radPropX>\
			bool operator != (const ::PropertyBaseGet<_radPropT, _radPropX> &other)\
			{ return get() != other.get(); }\
			operator get_type() const { return get(); }\
			get_type operator -> () const { return get(); } \
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& operator = (set_type var) \
			{ set(var); return *this; } \
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& operator = (const RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& other) \
			{ set(other.get()); return *this; } \
			get_type get() const { return RAD_CLASS_FROM_MEMBER(_class, name, this)->RAD_PRIVATE_PROPERTY_GET_METHOD_NAME(name)(); }\
			void set(set_type var) { RAD_CLASS_FROM_MEMBER(_class, name, this)->RAD_PRIVATE_PROPERTY_SET_METHOD_NAME(name)(var); }\
		private:\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) () {}\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) (const RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)&) {}\
			friend _ctype _class;\
	}

#define RAD_PRIVATE_PROPERTY_DECLARE_GETSET_CLASS_FOR_INCOMPLETE_TYPE(_ctype, _class, name, get_type, set_type) \
	class RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) : public \
	::PropertyBaseGetSet<RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name), get_type, set_type> { \
		public: \
			template <typename _radPropT, typename _radPropX>\
			bool operator == (const ::PropertyBaseGet<_radPropT, _radPropX> &other)\
			{ return get() == other.get(); }\
			template <typename _radPropT, typename _radPropX>\
			bool operator != (const ::PropertyBaseGet<_radPropT, _radPropX> &other)\
			{ return get() != other.get(); }\
			operator get_type() const;\
			get_type operator -> () const;\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& operator = (set_type var);\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& operator = (const RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& other) \
			{ set(other.get()); return *this; } \
			get_type get() const;\
			void set(set_type var);\
		private:\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) () {}\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) (const RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)&) {}\
			friend _ctype _class;\
	}

#define RAD_PRIVATE_PROPERTY_IMPLEMENT_GETSET_CLASS(_class, name, get_type, set_type)\
	inline _class::RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)::operator get_type() const { return get(); }\
	inline get_type _class::RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)::operator -> () const { get();  }\
	inline get_type _class::RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)::get() const { return RAD_CLASS_FROM_MEMBER(_class, name, this)->RAD_PRIVATE_PROPERTY_GET_METHOD_NAME(name)(); }\
	inline _class::RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& _class::RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)::operator = (set_type var) { set(var); return *this; }\
	inline void _class::RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)::set(set_type var) { RAD_CLASS_FROM_MEMBER(_class, name, this)->RAD_PRIVATE_PROPERTY_SET_METHOD_NAME(name)(var); }

#define RAD_PRIVATE_PROPERTY_DECLARE_GET_CLASS(_ctype, _class, name, type) \
	class RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) : public \
	::PropertyBaseGet<RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name), type> { \
		public: \
			template <typename _radPropT, typename _radPropX>\
			bool operator == (const ::PropertyBaseGet<_radPropT, _radPropX> &other)\
			{ return get() == other.get(); }\
			template <typename _radPropT, typename _radPropX>\
			bool operator != (const ::PropertyBaseGet<_radPropT, _radPropX> &other)\
			{ return get() != other.get(); }\
			operator type() const { return get(); }\
			type operator -> () const { return get(); } \
			type get() const { return RAD_CLASS_FROM_MEMBER(_class, name, this)->RAD_PRIVATE_PROPERTY_GET_METHOD_NAME(name)(); }\
		private:\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) () {}\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) (const RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)&) {}\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& operator = (const RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)&) { return *this; }\
			friend _ctype _class;\
	}

#define RAD_PRIVATE_PROPERTY_DECLARE_GET_CLASS_FOR_INCOMPLETE_TYPE(_ctype, _class, name, type) \
	class RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) : public \
		::PropertyBaseGet<RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name), type> { \
		public: \
			template <typename _radPropT, typename _radPropX>\
			bool operator == (const ::PropertyBaseGet<_radPropT, _radPropX> &other)\
			{ return get() == other.get(); }\
			template <typename _radPropT, typename _radPropX>\
			bool operator != (const ::PropertyBaseGet<_radPropT, _radPropX> &other)\
			{ return get() != other.get(); }\
			operator type() const;\
			type operator -> () const;\
			type get() const;\
		private:\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) () {}\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) (const RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)&) {}\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& operator = (const RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)&) { return *this; }\
			friend _ctype _class;\
	}

#define RAD_PRIVATE_PROPERTY_IMPLEMENT_GET_CLASS(_class, name, type)\
	inline _class::RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)::operator type() const { return get(); }\
	inline type _class::RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)::operator -> () const { return get(); }\
	inline type _class::RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)::get() const { return RAD_CLASS_FROM_MEMBER(_class, name, this)->RAD_PRIVATE_PROPERTY_GET_METHOD_NAME(name)(); }

#define RAD_PRIVATE_PROPERTY_DECLARE_SET_CLASS(_ctype, _class, name, type) \
	class RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) : public \
	::PropertyBaseSet<RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name), type> {\
		public: \
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& operator = (type var) \
			{ set(var); return *this; } \
			void set(type var) { RAD_CLASS_FROM_MEMBER(_class, name, this)->RAD_PRIVATE_PROPERTY_SET_METHOD_NAME(name)(var); }\
		private:\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) () {}\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) (const RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)&) {}\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& operator = (const RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)&) { return *this; }\
			friend _ctype _class;\
	}

#define RAD_PRIVATE_PROPERTY_DECLARE_SET_CLASS_FOR_INCOMPLETE_TYPE(_ctype, _class, name, type) \
	class RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) : public \
	::PropertyBaseSet<RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name), type> {\
		public: \
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& operator = (type var);\
			void set(type var);\
		private:\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) () {}\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) (const RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)&) {}\
			RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& operator = (const RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)&) { return *this; }\
			friend _ctype _class;\
	}

#define RAD_PRIVATE_PROPERTY_IMPLEMENT_SET_CLASS(_class, name, type)\
	inline _class::RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)& _class::RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)::operator = (type var) { set(var); return *this; }\
	inline void _class::RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)::set(type var) { RAD_CLASS_FROM_MEMBER(_class, name, this)->RAD_PRIVATE_PROPERTY_SET_METHOD_NAME(name)(var); }


#define RAD_PRIVATE_PROPERTY_DECLARE_GET(name, type) type RAD_PRIVATE_PROPERTY_GET_METHOD_NAME(name)() const
#define RAD_PRIVATE_PROPERTY_DECLARE_SET(name, type) void RAD_PRIVATE_PROPERTY_SET_METHOD_NAME(name)(type value)
#define RAD_PRIVATE_PROPERTY_IMPLEMENT_GET(name) RAD_PRIVATE_PROPERTY_GET_METHOD_NAME(name)() const
#define RAD_PRIVATE_PROPERTY_IMPLEMENT_SET(name) RAD_PRIVATE_PROPERTY_SET_METHOD_NAME(name)
#define RAD_PRIVATE_PROPERTY_DECLARE_FRIENDS(name) friend class RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name)

