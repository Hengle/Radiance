// RefCount.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if defined(RAD_OPT_DEBUG)
#include <limits>
#endif

#include "../PushSystemMacros.h"

template <typename T, typename TTraits>
inline RefCount<T, TTraits>::RefCount(ValueType init) :
m_refCount(init)
{
}

template <typename T, typename TTraits>
inline RefCount<T, TTraits>::~RefCount()
{
}

template <typename T, typename TTraits>
inline typename TTraits::ValueType RefCount<T, TTraits>::RAD_IMPLEMENT_GET(refCount)
{
	return TTraits::ToValue(m_refCount);
}


template <typename T, typename TTraits>
inline typename TTraits::ValueType RefCount<T, TTraits>::IncrementRefCount()
{
	RAD_ASSERT(m_refCount >= 0);
	RAD_DEBUG_ONLY(AssertOverflow());
	return TTraits::ToValue(++m_refCount);
}

template <typename T, typename TTraits>
inline typename TTraits::ValueType RefCount<T, TTraits>::DecrementRefCount()
{
	RAD_ASSERT(m_refCount > 0);
	RAD_DEBUG_ONLY(AssertUnderflow());
	ValueType result = TTraits::ToValue(--m_refCount);
	if (result == 0) { OnZeroReferences(); }
	return result;
}

#if defined(RAD_OPT_DEBUG)

template <typename T, typename TTraits>
inline void RefCount<T, TTraits>::AssertOverflow()
{
	RAD_ASSERT_MSG(TTraits::ToValue(m_refCount) < std::numeric_limits<ValueType>::max(), "Refcount overflow!" );
}


template <typename T, typename TTraits>
inline void RefCount<T, TTraits>::AssertUnderflow()
{
	RAD_ASSERT_MSG(TTraits::ToValue(m_refCount) > std::numeric_limits<ValueType>::min(), "Refcount underflow!" );
}

#endif

template<typename T>
inline SharedObjectHandle<T>::SharedObjectHandle()
{
	m_pRef = 0;
}

template<typename T>
inline SharedObjectHandle<T>::SharedObjectHandle(T* pRef) :
m_pRef(pRef)
{
}

template<typename T>
inline SharedObjectHandle<T>::SharedObjectHandle(T& ref) :
m_pRef(&ref)
{
}

template<typename T>
inline SharedObjectHandle<T>::SharedObjectHandle(const SharedObjectHandle<T> &hRef) :
m_pRef(hRef.m_pRef)
{
	Reference();
}

template<typename T>
inline SharedObjectHandle<T>::~SharedObjectHandle()
{
	Close();
	RAD_ASSERT(m_pRef == 0);
}

template<typename T>
inline void SharedObjectHandle<T>::Close()
{
	if (m_pRef) { Release(); }
}

template<typename T>
inline SharedObjectHandle<T>& SharedObjectHandle<T>::operator = (T* pRef)
{
	Attach(pRef, false);
	return *this;
}

template<typename T>
inline SharedObjectHandle<T>& SharedObjectHandle<T>::operator = (T& pRef)
{
	Attach(&pRef, false);
	return *this;
}

template<typename T>
inline SharedObjectHandle<T>& SharedObjectHandle<T>::operator = (const SharedObjectHandle<T>& hRef)
{
	Attach(hRef.m_pRef, true);
	return *this;
}

template<typename T>
inline T& SharedObjectHandle<T>::operator * () const
{
	RAD_ASSERT(m_pRef);
	return *m_pRef;
}

template<typename T>
inline T *SharedObjectHandle<T>::Ptr() const
{
	return m_pRef;
}

template<typename T>
inline T* SharedObjectHandle<T>::operator -> () const
{
	RAD_ASSERT(m_pRef);
	return m_pRef;
}

template<typename T>
inline bool SharedObjectHandle<T>::IsValid() const
{
	return m_pRef != 0;
}

template<typename T>
inline bool SharedObjectHandle<T>::operator == (const T* pRef) const
{
	return m_pRef == pRef;
}

template<typename T>
inline bool SharedObjectHandle<T>::operator == (const SharedObjectHandle<T>& hRef) const
{
	return m_pRef == hRef.m_pRef;
}

template<typename T>
inline bool SharedObjectHandle<T>::operator != (const T* pRef) const
{
	return m_pRef != pRef;
}

template<typename T>
inline bool SharedObjectHandle<T>::operator != (const SharedObjectHandle<T>& hRef) const
{
	return m_pRef != hRef.m_pRef;
}

template<typename T>
inline bool SharedObjectHandle<T>::operator ! () const
{
	return m_pRef == 0;
}

template<typename T>
inline SharedObjectHandle<T>::operator UnspecifiedBoolType() const
{
	return m_pRef == 0 ? 0 : &SelfType::m_pRef;
}

template<typename T>
inline void SharedObjectHandle<T>::Attach(T* pRef, bool addRef)
{
	if (m_pRef != pRef)
	{
		Close();
		m_pRef = pRef;
		if (addRef)
		{
			Reference();
		}
	}
}

template<typename T>
inline void SharedObjectHandle<T>::Release()
{
	RAD_ASSERT(m_pRef);
	m_pRef->DecrementRefCount();
	m_pRef = 0;
}

template <typename T>
template <typename TCast>
inline SharedObjectHandle<TCast> SharedObjectHandle<T>::StaticCast() const
{
	SharedObjectHandle<TCast> cast;
	cast.Attach(static_cast<TCast *>(const_cast<T*>(m_pRef)), true);
	return cast;
}

template <typename T>
template <typename TCast>
inline SharedObjectHandle<TCast> SharedObjectHandle<T>::DynamicCast() const
{
	SharedObjectHandle<TCast> cast;
	cast.Attach(dynamic_cast<TCast *>(const_cast<T*>(m_pRef)), true);
	return cast;
}

template <typename T>
template <typename TCast>
inline SharedObjectHandle<TCast> SharedObjectHandle<T>::ConstCast() const
{
	SharedObjectHandle<TCast> cast;
	cast.Attach(const_cast<TCast *>(const_cast<T*>(m_pRef)), true);
	return cast;
}

template <typename T>
inline void SharedObjectHandle<T>::Reference()
{
	if (m_pRef)
	{
		m_pRef->IncrementRefCount();
	}
}

#include "../PopSystemMacros.h"

