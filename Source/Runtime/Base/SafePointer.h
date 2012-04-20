// SafePointer.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Base.h"
#include "RefCount.h"
#include "../PushPack.h"

template<typename TPointer, typename TCounter = UReg>
class SafePointer : public RefCount<TCounter>
{
public:

	typedef RefCount<TCounter> SuperType;

	SafePointer();
	SafePointer(TPointer* ptr);
	virtual ~SafePointer();

	TPointer* Ptr() const;
	void  SetPtr(TPointer* ptr);

protected:

	// ReferenceCount
	virtual void OnZeroReferences() = 0;

private:

	TPointer* m_ptr;

	SafePointer(const SafePointer<TPointer, TCounter>&);
	SafePointer<TPointer, TCounter>& operator = (const SafePointer<TPointer, TCounter>&);
};

template<typename TPointer>
class SafePointerHandle
{
public:
	SafePointerHandle();
	SafePointerHandle(const SafePointerHandle<TPointer>& sfp);
	SafePointerHandle(SafePointer<TPointer>* ptr);
	~SafePointerHandle();

	// Explicit release

	void Release();

	// Pointer operations

	TPointer& operator*() const;
	TPointer* operator->() const;

	// Assignment

	SafePointerHandle<TPointer>& operator=(SafePointer<TPointer>* ptr);
	SafePointerHandle<TPointer>& operator=(const SafePointerHandle<TPointer>& ptr);
	
	// Equality
	bool operator == (const TPointer* ptr) const;
	bool operator == (const SafePointer<TPointer>* ptr) const;
	bool operator == (const SafePointerHandle<TPointer>& ptr) const;
	// Non-Equality
	bool operator != (const TPointer* ptr) const;
	bool operator != (const SafePointer<TPointer>* ptr) const;
	bool operator != (const SafePointerHandle<TPointer>& ptr) const;
		
	// Boolean operations

	bool operator!() const;
	operator bool() const;

	// returns true if referencing a non-null safe pointer.
	bool IsValid() const;
	// returns true if referencing a safe pointer, regardless of value.
	bool IsAttached() const;
	
	// Attach

	void Attach(const SafePointerHandle<TPointer>& ptr);
	void Attach(SafePointer<TPointer>* ptr);

private:

	SharedObjectHandle< SafePointer<TPointer> > m_safeEntity;

};

#include "../PopPack.h"
#include "SafePointer.inl"
