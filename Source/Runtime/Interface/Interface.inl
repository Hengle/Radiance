// InterfaceHandle.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

template <typename I>
inline InterfaceHandleBase<I>::InterfaceHandleBase() :
super_type()
{
}

template <typename I>
inline InterfaceHandleBase<I>::InterfaceHandleBase(const InterfaceHandleBase<I> &other) :
super_type(other)
{
}

template <typename I>
inline InterfaceHandleBase<I>::InterfaceHandleBase(void *p) :
super_type(p)
{
}

template <typename I>
inline InterfaceHandleBase<I>::~InterfaceHandleBase()
{
}

template <typename I>
inline InterfaceHandleBase<I> &InterfaceHandleBase<I>::operator=(const InterfaceHandleBase<I> &other)
{
	super_type::operator=(other);
	return *this;
}

template <typename I>
inline InterfaceHandleBase<I> &InterfaceHandleBase<I>::operator=(void *p)
{
	super_type::operator=(p);
	return *this;
}

inline void InterfaceHandleBase<IInterface>::Close()
{
	Release();
}

inline const char *InterfaceHandleBase<IInterface>::CID() const
{
	if (NULL != m_pData) { return GetData<IInterface>()->CID(); }
	return NULL;
}

inline const void *InterfaceHandleBase<IInterface>::Component() const
{
	if (NULL != m_pData) { return GetData<IInterface>()->Component(); }
	return NULL;
}

inline bool InterfaceHandleBase<IInterface>::operator !() const
{
	return (NULL == m_pData);
}

inline InterfaceHandleBase<IInterface>::operator UnspecifiedBoolType() const
{
	return m_pData == NULL ? NULL : &SelfType::m_pData;
}

inline InterfaceHandleBase<IInterface>::InterfaceHandleBase() :
m_pData(NULL)
{
}

inline InterfaceHandleBase<IInterface>::InterfaceHandleBase(const InterfaceHandleBase<IInterface> &other) :
m_pData(other.m_pData)
{
	Reference();
}

inline InterfaceHandleBase<IInterface>::InterfaceHandleBase(void *p) :
m_pData(p)
{
}

inline InterfaceHandleBase<IInterface>::~InterfaceHandleBase()
{
	Release();
}

inline InterfaceHandleBase<IInterface> &InterfaceHandleBase<IInterface>::operator=(const InterfaceHandleBase<IInterface> &other)
{
	if (other.m_pData != m_pData)
	{
		Release();
		m_pData = other.m_pData;
		Reference();
	}
	return *this;
}

inline InterfaceHandleBase<IInterface> &InterfaceHandleBase<IInterface>::operator=(void *p)
{
	Release();
	m_pData = p;
	return *this;
}

inline void *InterfaceHandleBase<IInterface>::Interface(const char *iid) const
{
	if (m_pData) { return GetData<IInterface>()->Interface(iid); }
	else         { return NULL; }
}

template <typename T>
inline T *InterfaceHandleBase<IInterface>::GetData() const
{
	RAD_ASSERT(NULL != m_pData);
	return reinterpret_cast<T *>(m_pData);
}

inline void InterfaceHandleBase<IInterface>::Reference()
{
	if (m_pData) { GetData<IInterface>()->Reference(); }
}

inline void InterfaceHandleBase<IInterface>::Release()
{
	if (m_pData)
	{
		void *p = m_pData; // avoid recursion.
		m_pData = NULL;
		reinterpret_cast<IInterface*>(p)->Release();
	}
}

template <typename I>
inline InterfaceHandle<I>::InterfaceHandle() :
super_type()
{
}

template <typename I>
inline InterfaceHandle<I>::InterfaceHandle(const InterfaceHandleBase<I> &other) :
super_type(other)
{
}

template <typename I>
inline InterfaceHandle<I>::InterfaceHandle(void *p) :
super_type(p)
{
}

template <typename I>
inline InterfaceHandle<I>::~InterfaceHandle()
{
}

template <typename I>
inline InterfaceHandle<I> &InterfaceHandle<I>::operator=(const InterfaceHandleBase<I> &other)
{
	super_type::operator=(other);
	return *this;
}

template <typename I>
inline InterfaceHandle<I> &InterfaceHandle<I>::operator=(void *p)
{
	super_type::operator=(p);
	return *this;
}

template <typename I>
inline I *InterfaceHandle<I>::operator->() const
{
	return InterfaceHandleBase<IInterface>::GetData<I>();
}

template <typename I>
template <typename I2>
inline InterfaceHandle<I2> InterfaceHandle<I>::Cast() const
{
	return InterfaceHandle<I2>(InterfaceHandleBase<IInterface>::Interface(I2::IID()));
}

template <typename I>
template <typename I2>
inline InterfaceHandle<I2> InterfaceHandle<I>::SafeCast() const
{
	InterfaceHandle<I2> i = this->Cast<I2>();
	RAD_VERIFY(i);
	return i;
}

inline InterfaceHandle<IInterface>::InterfaceHandle() :
InterfaceHandleBase<IInterface>()
{
}

inline InterfaceHandle<IInterface>::InterfaceHandle(const InterfaceHandleBase<IInterface> &other) :
InterfaceHandleBase<IInterface>(other)
{
}

inline InterfaceHandle<IInterface>::InterfaceHandle(void *p) :
InterfaceHandleBase<IInterface>(p)
{
}

inline InterfaceHandle<IInterface>::~InterfaceHandle()
{
}

inline InterfaceHandle<IInterface> &InterfaceHandle<IInterface>::operator=(const InterfaceHandleBase<IInterface> &other)
{
	super_type::operator=(other);
	return *this;
}

inline InterfaceHandle<IInterface> &InterfaceHandle<IInterface>::operator=(void *p)
{
	super_type::operator=(p);
	return *this;
}

inline IInterface *InterfaceHandle<IInterface>::operator->() const
{
	return this->GetData<IInterface>();
}

template <typename I>
inline InterfaceHandle<I> InterfaceHandle<IInterface>::Cast() const
{
	return InterfaceHandle<I>(InterfaceHandleBase<IInterface>::Interface(I::IID()));
}

template <typename I>
inline InterfaceHandle<I> InterfaceHandle<IInterface>::SafeCast() const
{
	InterfaceHandle<I> i = this->Cast<I>();
	RAD_VERIFY(i);
	return i;
}
