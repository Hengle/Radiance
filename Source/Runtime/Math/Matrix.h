// Matrix.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "MatrixDef.h"
#include "Vector.h"
#include "Euler.h"
#include "Quaternion.h"
#include "../PushPack.h"

#define __RADMATH_MATRIX_H__ // so we know it's here


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math free functions
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Vector4<T> operator*(const Vector4<T> &v, const Rows4X4<T> &r);

template <typename T>
T Determinant(T a, T b, T c, T d);

template <typename T>
T Determinant(
	T a1, T a2, T a3,
	T b1, T b2, T b3,
	T c1, T c2, T c3
);

template <typename T>
T Determinant(
	T a1, T a2, T a3, T a4,
	T b1, T b2, T b3, T b4,
	T c1, T c2, T c3, T c4,
	T d1, T d2, T d3, T d4
);

//////////////////////////////////////////////////////////////////////////////////////////
// math::Scale3<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Scale3 : 
public Vector3<T>
{

public:

	// Constructors

	Scale3();
	Scale3(T s);
	explicit Scale3(const T *v);
	Scale3(T xx, T yy, T zz);
	Scale3(const Vector2<T> &v, T zz);
	Scale3(const Vector3<T> &v);

	// Destructor

	~Scale3();

	// Operations

	Vector3<T> Scale(const Vector3<T> &v) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// math::Rows4X4<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Rows4X4
{

public:

	// Types

	typedef T ValueType;

	// Constructors

	Rows4X4();
	Rows4X4(
		const Vector4<T> &row0,
		const Vector4<T> &row1,
		const Vector4<T> &row2,
		const Vector4<T> &row3
	);
	explicit Rows4X4(const Columns4X4<T> &columns);
	Rows4X4(const Rows4X4<T> &rows);

	// Destructor

	~Rows4X4();

	// Access

	const Vector4<T> &operator[](int i) const;
	Vector4<T> &operator[](int i);
	const Vector4<T> &Row(int i) const;
	Vector4<T> Column(int i) const;

	// Operations

	Rows4X4<T> &Initialize(
		const Vector4<T> &row0,
		const Vector4<T> &row1,
		const Vector4<T> &row2,
		const Vector4<T> &row3
	);

	Rows4X4<T> &Initialize(
		const Vector3<T> &row0,
		const Vector3<T> &row1,
		const Vector3<T> &row2,
		const Vector3<T> &row3
	);

	// Assignment operations

	Rows4X4<T> &operator=(const Rows4X4<T> &rows);
	Rows4X4<T> &operator=(const Columns4X4<T> &columns);

private:

	Vector4<T> m_row0;
	Vector4<T> m_row1;
	Vector4<T> m_row2;
	Vector4<T> m_row3;

};

//////////////////////////////////////////////////////////////////////////////////////////
// math::Columns4X4<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Columns4X4
{

public:

	// Types

	typedef T ValueType;

	// Constructors

	Columns4X4();
	Columns4X4(
		const Vector4<T> &col0,
		const Vector4<T> &col1,
		const Vector4<T> &col2,
		const Vector4<T> &col3
	);
	explicit Columns4X4(const Rows4X4<T> &rows);
	Columns4X4(const Columns4X4<T> &columns);

	// Destructor

	~Columns4X4();

	// Access

	const Vector4<T> &operator[](int i) const;
	Vector4<T> &operator[](int i);
	Vector4<T> Row(int i) const;
	const Vector4<T> &Column(int i) const;
	
	// Operations

	Columns4X4<T> &Initialize(
		const Vector4<T> &col0,
		const Vector4<T> &col1,
		const Vector4<T> &col2,
		const Vector4<T> &col3
	);

	Columns4X4<T> &Initialize(
		const Vector4<T> &col0,
		const Vector4<T> &col1,
		const Vector4<T> &col2
	);

	// Assignment operations

	Columns4X4<T> &operator=(const Rows4X4<T> &rows);
	Columns4X4<T> &operator=(const Columns4X4<T> &columns);

private:

	Vector4<T> m_col0;
	Vector4<T> m_col1;
	Vector4<T> m_col2;
	Vector4<T> m_col3;

};

//////////////////////////////////////////////////////////////////////////////////////////
// math matrix backends
//////////////////////////////////////////////////////////////////////////////////////////

#include "Generic/GenericMatrixBackend.h"

//////////////////////////////////////////////////////////////////////////////////////////
// select the backend
//////////////////////////////////////////////////////////////////////////////////////////

// Specialize this below for platform-specific backend selection.
template <typename T>
struct MatrixBackendSelector
{
	typedef GenericMatrix4X4Traits<Matrix4X4, T>       Matrix4X4TraitsType;
	typedef GenericMatrix4X4Traits<AffineMatrix4X4, T> AffineMatrix4X4TraitsType;
	typedef GenericMatrix4X4Traits<RigidMatrix4X4, T>  RigidMatrix4X4TraitsType;
	typedef GenericMatrix4X4Traits<SolidMatrix4X4, T>  SolidMatrix4X4TraitsType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// math matrix types
//////////////////////////////////////////////////////////////////////////////////////////

#include "Matrix4X4.h"
#include "AffineMatrix4X4.h"
#include "RigidMatrix4X4.h"
#include "SolidMatrix4X4.h"

} // math


#include "../PopPack.h"
#include "Matrix.inl"
