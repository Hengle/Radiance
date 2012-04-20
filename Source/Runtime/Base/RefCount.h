// RefCount.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Base.h"
#include "RefCountDef.h"
#include "../Thread/Interlocked.h"
#include "../PushPack.h"

template <typename T = UReg, typename TTraits = ValueTraits<T> >
class RefCount
{
public:

	typedef TTraits                     TraitsType;
	typedef typename TTraits::ValueType ValueType;

	RefCount(ValueType init = ValueType(1));
	virtual ~RefCount();

	RAD_DECLARE_READONLY_PROPERTY(RefCount, refCount, ValueType);

	ValueType IncrementRefCount();
	ValueType DecrementRefCount();

protected:

	virtual void OnZeroReferences() = 0;

private:

	RAD_DECLARE_GET(refCount, ValueType);

	RAD_DEBUG_ONLY(void AssertOverflow());
	RAD_DEBUG_ONLY(void AssertUnderflow());

	T m_refCount;
};

typedef RefCount<
	::thread::Interlocked<UReg>,
	::thread::InterlockedValueTraits<UReg>
> AtomicRefCount;

template<typename T>
class SharedObjectHandle
{
	typedef SharedObjectHandle<T> SelfType;

public:

	SharedObjectHandle();
	SharedObjectHandle(const SharedObjectHandle<T> &handle);
	explicit SharedObjectHandle(T *object);
	explicit SharedObjectHandle(T &object);
	~SharedObjectHandle();

	// Close the handle

	void Close();

	// Pointer operations

	T &operator *() const;
	T *operator ->() const;
	T *Ptr() const;

	// Assignment

	SharedObjectHandle<T>& operator =(T *object);
	SharedObjectHandle<T>& operator =(T &object);
	SharedObjectHandle<T>& operator =(const SharedObjectHandle<T> &handle);

	// Equality

	bool operator == (const T *object) const;
	bool operator == (const SharedObjectHandle<T> &handle) const;

	// Non-Equality

	bool operator != (const T *object) const;
	bool operator != (const SharedObjectHandle<T> &handle) const;

	// Boolean operations

	bool operator !() const;

	typedef T *SelfType::*UnspecifiedBoolType;
    operator UnspecifiedBoolType() const;

	bool IsValid() const;

	// Attach and Release

	void Attach(T *object, bool addRef);
	void Release();

	// Casts

	template <typename TCast>
	SharedObjectHandle<TCast> StaticCast() const;

	template <typename TCast>
	SharedObjectHandle<TCast> DynamicCast() const;

	template <typename TCast>
	SharedObjectHandle<TCast> ConstCast() const;

private:

	void Reference();

	T *m_pRef;
};

#include "../PopPack.h"
#include "RefCount.inl"

