// SafePointer.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

template<typename TPointer>
inline SafePointerHandle<TPointer>::SafePointerHandle() :
m_safeEntity()
{
}

template<typename TPointer>
inline SafePointerHandle<TPointer>::SafePointerHandle(const SafePointerHandle<TPointer>& sfp)
{
	m_safeEntity = sfp.m_safeEntity;
}

template<typename TPointer>
inline SafePointerHandle<TPointer>::SafePointerHandle(SafePointer<TPointer>* ptr)
{
	m_safeEntity.Attach(ptr, false);
}

template<typename TPointer>
inline SafePointerHandle<TPointer>::~SafePointerHandle()
{
	m_safeEntity.Close();
}

template<typename TPointer>
inline void SafePointerHandle<TPointer>::Release()
{
	m_safeEntity.Release();
}

template<typename TPointer>
inline TPointer& SafePointerHandle<TPointer>::operator* () const
{
	RAD_ASSERT_MSG(IsValid(), "Illegal access to unbound SafePointerHandle!");
	return *(m_safeEntity->Ptr());
}

template<typename TPointer>
inline TPointer* SafePointerHandle<TPointer>::operator-> () const
{
	RAD_ASSERT_MSG(IsValid(), "Illegal access to unbound SafePointerHandle!");
	return m_safeEntity->Ptr();
}

template<typename TPointer>
inline SafePointerHandle<TPointer>& SafePointerHandle<TPointer>::operator=(SafePointer<TPointer>* ptr)
{
	Attach(ptr);
	return *this;
}

template<typename TPointer>
inline SafePointerHandle<TPointer>& SafePointerHandle<TPointer>::operator=(const SafePointerHandle<TPointer>& ptr)
{
	Attach(ptr);
	return *this;
}


template<typename TPointer>
inline bool SafePointerHandle<TPointer>::operator == (const TPointer* ptr) const
{
	return (IsValid() && (m_safeEntity->Ptr()==ptr)) || (!IsValid() && (ptr==0));
}

template<typename TPointer>
inline bool SafePointerHandle<TPointer>::operator == (const SafePointer<TPointer>* ptr) const
{
	return (IsValid() && (m_safeEntity->Ptr() == ptr->Ptr())) || (!IsValid() && (ptr->Ptr()==0));
}

template<typename TPointer>
bool SafePointerHandle<TPointer>::operator == (const SafePointerHandle<TPointer>& ptr) const // not inlined on purpose!
{
	if (IsValid() && ptr.IsValid())
	{
		return (m_safeEntity->Ptr() == (ptr.m_safeEntity->Ptr()));
	}
	else
	{
		return (!IsValid() && !ptr.IsValid());
	}
}


template<typename TPointer>
inline bool SafePointerHandle<TPointer>::operator != (const TPointer* ptr) const
{
	return !(*this == ptr);
}

template<typename TPointer>
inline bool SafePointerHandle<TPointer>::operator != (const SafePointer<TPointer>* ptr) const
{
	return !(*this == ptr);
}

template<typename TPointer>
inline bool SafePointerHandle<TPointer>::operator != (const SafePointerHandle<TPointer>& ptr) const
{
	return !(*this == ptr);
}

template<typename TPointer>
inline bool SafePointerHandle<TPointer>::operator!() const
{
	return !IsValid();
}

template<typename TPointer>
inline SafePointerHandle<TPointer>::operator bool() const
{
	return IsValid();
}

template<typename TPointer>
inline bool SafePointerHandle<TPointer>::IsValid() const
{
	return IsAttached() && (m_safeEntity->Ptr() != 0);
}

template<typename TPointer>
inline bool SafePointerHandle<TPointer>::IsAttached() const
{
	return m_safeEntity.IsValid();
}

template<typename TPointer>
inline void SafePointerHandle<TPointer>::Attach(const SafePointerHandle<TPointer>& ptr)
{
	m_safeEntity = ptr.m_safeEntity;
}

template<typename TPointer>
inline void SafePointerHandle<TPointer>::Attach(SafePointer<TPointer>* ptr)
{
	m_safeEntity.Attach(ptr);
}

template<typename TPointer, typename TCounter>
inline SafePointer<TPointer, TCounter>::SafePointer() :
m_ptr(0)
{
}

template<typename TPointer, typename TCounter>
inline SafePointer<TPointer, TCounter>::SafePointer(TPointer* ptr) : m_ptr(ptr)
{
}

template<typename TPointer, typename TCounter>
inline SafePointer<TPointer, TCounter>::~SafePointer()
{
	m_ptr = 0;
}

template<typename TPointer, typename TCounter>
inline TPointer* SafePointer<TPointer, TCounter>::Ptr() const
{
	return m_ptr;
}

template<typename TPointer, typename TCounter>
inline void SafePointer<TPointer, TCounter>::SetPtr(TPointer* ptr)
{
	m_ptr = ptr;
}
