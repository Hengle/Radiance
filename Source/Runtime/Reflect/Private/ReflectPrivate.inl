// ReflectPrivate.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#include "../../Meta/TypeTraits/FunctionTraits.h"


namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodArgumentsMatchRecursive
//////////////////////////////////////////////////////////////////////////////////////////

template <typename MethodType, typename FunctionType>
struct MethodArgumentsMatchRecursive;

// 0 Args
template
<
	typename MethodType,
	typename R
>
struct MethodArgumentsMatchRecursive<MethodType, R ()>
{
	static bool Matches(const MethodType *method)
	{
		return method->ReturnType() == Type<R>();
	}
};

template
<
	typename R
>
struct MethodArgumentsMatchRecursive<Class::CONSTRUCTOR, R ()>
{
	static bool Matches(const Class::CONSTRUCTOR *method)
	{
		return Type<void>() == Type<R>();
	}
};

// 1 Args
template
<
	typename MethodType,
	typename R,
	typename T1
>
struct MethodArgumentsMatchRecursive<MethodType, R (T1)>
{
	static bool Matches(const MethodType *method)
	{
		return (method->Argument(0)->Type() == Type<T1>()) &&
			MethodArgumentsMatchRecursive<MethodType, R ()>::Matches(method);
	}
};

// 2 Args
template
<
	typename MethodType,
	typename R,
	typename T1,
	typename T2
>
struct MethodArgumentsMatchRecursive<MethodType, R (T1, T2)>
{
	static bool Matches(const MethodType *method)
	{
		return (method->Argument(1)->Type() == Type<T2>()) &&
			MethodArgumentsMatchRecursive<MethodType, R (T1)>::Matches(method);
	}
};

// 3 Args
template
<
	typename MethodType,
	typename R,
	typename T1,
	typename T2,
	typename T3
>
struct MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3)>
{
	static bool Matches(const MethodType *method)
	{
		return (method->Argument(2)->Type() == Type<T3>()) &&
			MethodArgumentsMatchRecursive<MethodType, R (T1, T2)>::Matches(method);
	}
};

// 4 Args
template
<
	typename MethodType,
	typename R,
	typename T1,
	typename T2,
	typename T3,
	typename T4
>
struct MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4)>
{
	static bool Matches(const MethodType *method)
	{
		return (method->Argument(3)->Type() == Type<T4>()) &&
			MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3)>::Matches(method);
	}
};

// 5 Args
template
<
	typename MethodType,
	typename R,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5
>
struct MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4, T5)>
{
	static bool Matches(const MethodType *method)
	{
		return (method->Argument(4)->Type() == Type<T5>()) &&
			MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4)>::Matches(method);
	}
};

// 6 Args
template
<
	typename MethodType,
	typename R,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6
>
struct MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4, T5, T6)>
{
	static bool Matches(const MethodType *method)
	{
		return (method->Argument(5)->Type() == Type<T6>()) &&
			MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4, T5)>::Matches(method);
	}
};

// 7 Args
template
<
	typename MethodType,
	typename R,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7
>
struct MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4, T5, T6, T7)>
{
	static bool Matches(const MethodType *method)
	{
		return (method->Argument(6)->Type() == Type<T7>()) &&
			MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4, T5, T6)>::Matches(method);
	}
};

// 8 Args
template
<
	typename MethodType,
	typename R,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8
>
struct MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4, T5, T6, T7, T8)>
{
	static bool Matches(const MethodType *method)
	{
		return (method->Argument(7)->Type() == Type<T8>()) &&
			MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4, T5, T6, T7)>::Matches(method);
	}
};

// 9 Args
template
<
	typename MethodType,
	typename R,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8,
	typename T9
>
struct MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4, T5, T6, T7, T8, T9)>
{
	static bool Matches(const MethodType *method)
	{
		return (method->Argument(8)->Type() == Type<T9>()) &&
			MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4, T5, T6, T7, T8)>::Matches(method);
	}
};

// 10 Args
template
<
	typename MethodType,
	typename R,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8,
	typename T9,
	typename T10
>
struct MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)>
{
	static bool Matches(const MethodType *method)
	{
		return (method->Argument(9)->Type() == Type<T10>()) &&
			MethodArgumentsMatchRecursive<MethodType, R (T1, T2, T3, T4, T5, T6, T7, T8, T9)>::Matches(method);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodArgumentsMatch
//////////////////////////////////////////////////////////////////////////////////////////

template <typename MethodType, typename FunctionType>
struct MethodArgumentsMatch
{
	static bool Matches(const MethodType *method)
	{
		return (method->NumArguments() == meta::FunctionTraits<FunctionType>::NUM_ARGS) &&
			MethodArgumentsMatchRecursive<MethodType, FunctionType>::Matches(method);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::FindAttribute()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename TChar>
const ATTRIBUTE *FindAttribute(const T *object, const TChar *name)
{
	RAD_ASSERT(object&&name);

	const int NUM_ATTRS = object->NumAttributes();
	for (int i = 0; i < NUM_ATTRS; ++i)
	{
		const ATTRIBUTE *attr = object->Attribute(i);
		if (0 == string::cmp(attr->Name<TChar>(), name))
		{
			return attr;
		}
	}

	return 0;
}

template<typename T>
const ATTRIBUTE *FindAttribute(const T *object, const Class *type)
{
	RAD_ASSERT(object&&type);

	const int NUM_ATTRS = object->NumAttributes();
	for (int i = 0; i < NUM_ATTRS; ++i)
	{
		const ATTRIBUTE *attr = object->Attribute(i);
		if (attr->Type() == type)
		{
			return attr;
		}
	}

	return 0;
}

template<typename T, typename TChar>
const ATTRIBUTE *FindAttribute(const T *object, const Class *type, const TChar *name)
{
	RAD_ASSERT(object&&type&&name);

	const int NUM_ATTRS = object->NumAttributes();
	for (int i = 0; i < NUM_ATTRS; ++i)
	{
		const ATTRIBUTE *attr = object->Attribute(i);
		if (attr->Type() == type)
		{
			if (0 == string::cmp(attr->Name<TChar>(), name))
			{
				return attr;
			}
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::AttributeValue()
//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline const ATTRIBUTE *AttributeValue(const T *object, const Class *type, ConstReflected &value)
{
	const ATTRIBUTE *attr = FindAttribute<T>(object, type);
	if (attr)
	{
		value = reflect::Reflect(attr);
	}

	return attr;
}

template<typename T, typename TChar>
inline const ATTRIBUTE *AttributeValue(const T *object, const TChar *name, ConstReflected &value)
{
	const ATTRIBUTE *attr = FindAttribute<T, TChar>(object, name);
	if (attr)
	{
		value = reflect::Reflect(attr);
	}

	return attr;
}

template<typename T, typename TChar>
inline const ATTRIBUTE *AttributeValue(const T *object, const Class *type, const TChar *name, ConstReflected &value)
{
	const ATTRIBUTE *attr = FindAttribute<T, TChar>(object, type, name);
	if (attr)
	{
		value = reflect::Reflect(attr);
	}

	return attr;
}

template<typename T, typename X>
inline const ATTRIBUTE *AttributeValue(const T *object, X &value)
{
	const ATTRIBUTE *attr = FindAttribute<T>(object, Type<X>());
	if (attr) { value = *(const X*)reflect::Reflect(attr); }
	return attr;
}

template<typename T, typename X, typename TChar>
inline const ATTRIBUTE *AttributeValue(const T *object, const TChar *name, X &value)
{
	const ATTRIBUTE *attr = FindAttribute<T>(object, Type<X>(), name);
	if (attr) { value = *(const X*)reflect::Reflect(attr); }
	return attr;
}

} // details
} // reflect

