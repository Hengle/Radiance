// Interface.h
// Component interface system
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "InterfaceDef.h"
#include "ReflectInterface.h"
#include "../PushPack.h"

RAD_INTERFACE(IInterface)
{
	RADREFLECT_EXPOSE_PRIVATES(IInterface)

public:

	typedef IInterface SUPER;
	typedef InterfaceHandle<IInterface> HANDLE;

	static const char *IID() { return "rad.IInterface"; }

	virtual const char *CID() = 0;
	virtual const void *Component() = 0;
	virtual void *Interface(const char *iid) = 0;
	virtual UReg Reference() = 0;
	virtual UReg Release() = 0;

};

template <typename I>
class InterfaceHandleBase :
public InterfaceHandleBase<typename I::SUPER>
{
protected:

	// Constructors

	InterfaceHandleBase();
	InterfaceHandleBase(const InterfaceHandleBase<I> &other);
	InterfaceHandleBase(void *p);

	// Destructor

	~InterfaceHandleBase();

	// Assignment

	InterfaceHandleBase<I> &operator=(const InterfaceHandleBase<I> &other);
	InterfaceHandleBase<I> &operator=(void *p);

private:

	friend class InterfaceHandle<I>;
	typedef InterfaceHandleBase<typename I::SUPER> super_type;

};

template <>
class InterfaceHandleBase<IInterface>
{
	RADREFLECT_EXPOSE_PRIVATES(InterfaceHandle<IInterface>)

	typedef InterfaceHandle<IInterface> SelfType;

public:

	// Close the handle

	void Close();

	// Component identity

	const char *CID() const;
	const void *Component() const;

	// Extractors

	bool operator !() const;

	typedef void *SelfType::*UnspecifiedBoolType;
    operator UnspecifiedBoolType() const;

	template <typename T>
	T *GetData() const;

protected:

	// Constructors

	InterfaceHandleBase();
	InterfaceHandleBase(const InterfaceHandleBase<IInterface> &other);
	InterfaceHandleBase(void *p);

	// Destructor

	~InterfaceHandleBase();

	// Assignment

	InterfaceHandleBase<IInterface> &operator=(const InterfaceHandleBase<IInterface> &other);
	InterfaceHandleBase<IInterface> &operator=(void *p);

private:

	template <typename T> friend class InterfaceHandle;

	void *Interface(const char *iid) const;

	void Reference();
	void Release();

	void *m_pData;
};

//////////////////////////////////////////////////////////////////////////////////////////
// rad::InterfaceHandle<I>
//////////////////////////////////////////////////////////////////////////////////////////
//
// Auto reference-counting and interface casting pointer
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename I>
class InterfaceHandle :
public InterfaceHandleBase<I>
{
	RADREFLECT_EXPOSE_PRIVATES(InterfaceHandle<I>)

public:

	// Constructors

	InterfaceHandle();
	InterfaceHandle(const InterfaceHandleBase<I> &other);
	InterfaceHandle(void *p);

	// Destructor

	~InterfaceHandle();

	// Assignment

	InterfaceHandle<I> &operator=(const InterfaceHandleBase<I> &other);
	InterfaceHandle<I> &operator=(void *p);

	// Extractors

	I *operator->() const;

	template <typename X>
	InterfaceHandle<X> Cast() const;
	template <typename X>
	InterfaceHandle<X> SafeCast() const;

private:

	typedef InterfaceHandleBase<I> super_type;

};

template <>
class InterfaceHandle<IInterface> :
public InterfaceHandleBase<IInterface>
{
	RADREFLECT_EXPOSE_PRIVATES(InterfaceHandle<IInterface>)

public:

	// Constructors

	InterfaceHandle();
	InterfaceHandle(const InterfaceHandleBase<IInterface> &other);
	InterfaceHandle(void *p);

	// Destructor

	~InterfaceHandle();

	// Assignment

	InterfaceHandle<IInterface> &operator=(const InterfaceHandleBase<IInterface> &other);
	InterfaceHandle<IInterface> &operator=(void *p);

	// Extractors

	IInterface *operator->() const;

	template <typename I>
	InterfaceHandle<I> Cast() const;
	template <typename I>
	InterfaceHandle<I> SafeCast() const;

private:

	typedef InterfaceHandleBase<IInterface> super_type;

};

#include "../PopPack.h"
#include "Interface.inl"
