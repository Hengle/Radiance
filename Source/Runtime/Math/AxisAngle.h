// AxisAngle.h
// Axis/Angle class
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Vector.h"
#include "AxisAngleDef.h"
#include "../PushPack.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math::AxisAngle<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class AxisAngle
{
public:

	// Constructors

	AxisAngle();
	AxisAngle(const Vector3<T> &axis, T radians);
	AxisAngle(const AxisAngle<T> &axisAngle);

	// Destructor

	~AxisAngle();

	// Access

	const Vector3<T> &Axis() const;
	void Axis(const Vector3<T> &axis);

	T Angle() const;
	void Angle(T radians);

	// In place operations

	AxisAngle<T> &Initialize(const Vector3<T> &unitAxis, T radians);

	// Assignment operations

	AxisAngle<T> &operator=(const AxisAngle<T> &axisAngle);

protected:

	Vector3<T> m_axis;
	T          m_angle;
};

//////////////////////////////////////////////////////////////////////////////////////////
// End namespace math
//////////////////////////////////////////////////////////////////////////////////////////

} // namespace math


#include "../PopPack.h"
#include "AxisAngle.inl"
