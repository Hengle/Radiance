// Euler.h
// Euler angle class
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Vector.h"
#include "EulerDef.h"
#include "../PushPack.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math::Euler<T>
//////////////////////////////////////////////////////////////////////////////////////////
//
// Assumes RH coordinate system with Y-up
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Euler : public Vector3<T>
{

public:

	// Constructors

	Euler();
	Euler(const T *v);
	Euler(T pitch, T yaw, T roll);
	Euler(const Euler<T> &e);
	explicit Euler(const Vector3<T> &v);

	// Destructor

	~Euler();
	
	// Access

	T Pitch() const;
	void Pitch(T pitch);
	T Yaw() const;
	void Yaw(T yaw);
	T Roll() const;
	void Roll(T roll);
};

} // math


#include "../PopPack.h"
#include "Euler.inl"
