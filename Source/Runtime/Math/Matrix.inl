// Matrix.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math::Determinant<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T Determinant(T a, T b, T c, T d)
{
	return (a * d - b * c);
}

template <typename T>
inline T Determinant(
	T a1, T a2, T a3,
	T b1, T b2, T b3,
	T c1, T c2, T c3
)
{
	return (
		a1 * Determinant(b2, b3, c2, c3) -
		b1 * Determinant(a2, a3, c2, c3) +
		c1 * Determinant(a2, a3, b2, b3)
	);
}

template <typename T>
inline T Determinant(
	T a1, T a2, T a3, T a4,
	T b1, T b2, T b3, T b4,
	T c1, T c2, T c3, T c4,
	T d1, T d2, T d3, T d4
)
{
	return (
		a1 * Determinant(b2, b3, b4, c2, c3, c4, d2, d3, d4) -
		b1 * Determinant(a2, a3, a4, c2, c3, c4, d2, d3, d4) +
		c1 * Determinant(a2, a3, a4, b2, b3, b4, d2, d3, d4) -
		d1 * Determinant(a2, a3, a4, b2, b3, b4, c2, c3, c4)
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Scale3<T>::Scale3()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Scale3<T>::Scale3() :
Vector3<T>()
{
}

template <typename T>
inline Scale3<T>::Scale3(T s) :
Vector3<T>(s, s, s)
{
}

template <typename T>
inline Scale3<T>::Scale3(const T *v) :
Vector3<T>(v)
{
}

template <typename T>
inline Scale3<T>::Scale3(T x, T y, T z) :
Vector3<T>(x, y, z)
{
}

template <typename T>
inline Scale3<T>::Scale3(const Vector2<T> &v, T z) :
Vector3<T>(v, z)
{
}

template <typename T>
inline Scale3<T>::Scale3(const Vector3<T> &v) :
Vector3<T>(v)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Scale3<T>::~Scale3()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Scale3<T>::~Scale3()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Scale3<T>::Scale()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T> Scale3<T>::Scale(const Vector3<T> &v) const
{
	return Vector3<T>(
		v.X() * this->X(),
		v.Y() * this->Y(),
		v.Z() * this->Z()
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Rows4X4<T>::Rows4X4()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Rows4X4<T>::Rows4X4()
{
}

template <typename T>
inline Rows4X4<T>::Rows4X4(
	const Vector4<T> &row0,
	const Vector4<T> &row1,
	const Vector4<T> &row2,
	const Vector4<T> &row3
) :
m_row0(row0),
m_row1(row1),
m_row2(row2),
m_row3(row3)
{
}

template <typename T>
inline Rows4X4<T>::Rows4X4(const Rows4X4 &rows) :
m_row0(rows.m_row0),
m_row1(rows.m_row1),
m_row2(rows.m_row2),
m_row3(rows.m_row3)
{
}

template <typename T>
inline Rows4X4<T>::Rows4X4(const Columns4X4<T> &columns) :
m_row0(columns[0][0], columns[1][0], columns[2][0], columns[3][0]),
m_row1(columns[0][1], columns[1][1], columns[2][1], columns[3][1]),
m_row2(columns[0][2], columns[1][2], columns[2][2], columns[3][2]),
m_row3(columns[0][3], columns[1][3], columns[2][3], columns[3][3])
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Rows4X4<T>::~Rows4X4()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Rows4X4<T>::~Rows4X4()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Rows4X4<T>::operator[]()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Vector4<T> &Rows4X4<T>::operator[](int i) const
{
	return (&m_row0)[i];
}

template <typename T>
inline Vector4<T> &Rows4X4<T>::operator[](int i)
{
	return (&m_row0)[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Rows4X4<T>::Row()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Vector4<T> &Rows4X4<T>::Row(int i) const
{
	return operator[](i);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Rows4X4<T>::Column()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector4<T> Rows4X4<T>::Column(int i) const
{
	return Vector4<T>(
		m_row0[i],
		m_row1[i],
		m_row2[i],
		m_row3[i]
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Rows4X4<T>::Column()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Rows4X4<T> &Rows4X4<T>::Initialize(
		const Vector4<T> &row0,
		const Vector4<T> &row1,
		const Vector4<T> &row2,
		const Vector4<T> &row3
)
{
	m_row0 = row0;
	m_row1 = row1;
	m_row2 = row2;
	m_row3 = row3;
	return *this;
}

template <typename T>
inline Rows4X4<T> &Rows4X4<T>::Initialize(
		const Vector3<T> &row0,
		const Vector3<T> &row1,
		const Vector3<T> &row2,
		const Vector3<T> &row3
)
{
	m_row0.Vector3<T>::operator=(row0);
	m_row1.Vector3<T>::operator=(row1);
	m_row2.Vector3<T>::operator=(row2);
	m_row3.Vector3<T>::operator=(row3);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Rows4X4<T>::operator=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Rows4X4<T> &Rows4X4<T>::operator=(const Rows4X4<T> &rows)
{
	m_row0 = rows.m_row0;
	m_row1 = rows.m_row1;
	m_row2 = rows.m_row2;
	m_row3 = rows.m_row3;
	return *this;
}

template <typename T>
inline Rows4X4<T> &Rows4X4<T>::operator=(const Columns4X4<T> &columns)
{
	m_row0.Initialize(columns[0][0], columns[1][0], columns[2][0], columns[3][0]);
	m_row1.Initialize(columns[0][1], columns[1][1], columns[2][1], columns[3][1]);
	m_row2.Initialize(columns[0][2], columns[1][2], columns[2][2], columns[3][2]);
	m_row3.Initialize(columns[0][3], columns[1][3], columns[2][3], columns[3][3]);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Columns4X4<T>::Columns4X4()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Columns4X4<T>::Columns4X4()
{
}

template <typename T>
inline Columns4X4<T>::Columns4X4(
	const Vector4<T> &col0,
	const Vector4<T> &col1,
	const Vector4<T> &col2,
	const Vector4<T> &col3
) :
m_col0(col0),
m_col1(col1),
m_col2(col2),
m_col3(col3)
{
}

template <typename T>
inline Columns4X4<T>::Columns4X4(const Rows4X4<T> &rows) :
m_col0(rows[0][0], rows[1][0], rows[2][0], rows[3][0]),
m_col1(rows[0][1], rows[1][1], rows[2][1], rows[3][1]),
m_col2(rows[0][2], rows[1][2], rows[2][2], rows[3][2]),
m_col3(rows[0][3], rows[1][3], rows[2][3], rows[3][3])
{
}

template <typename T>
inline Columns4X4<T>::Columns4X4(const Columns4X4<T> &columns) :
m_col0(columns.m_col0),
m_col1(columns.m_col1),
m_col2(columns.m_col2),
m_col3(columns.m_col3)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Columns4X4<T>::~Columns4X4()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Columns4X4<T>::~Columns4X4()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Columns4X4<T>::operator[]()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Vector4<T> &Columns4X4<T>::operator[](int i) const
{
	return (&m_col0)[i];
}

template <typename T>
inline Vector4<T> &Columns4X4<T>::operator[](int i)
{
	return (&m_col0)[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Columns4X4<T>::Row()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector4<T> Columns4X4<T>::Row(int i) const
{
	return Vector4<T>(
		m_col0[i],
		m_col1[i],
		m_col2[i],
		m_col3[i]
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Columns4X4<T>::Column()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Vector4<T> &Columns4X4<T>::Column(int i) const
{
	return operator[](i);	
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Columns4X4<T>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Columns4X4<T> &Columns4X4<T>::Initialize(
	const Vector4<T> &col0,
	const Vector4<T> &col1,
	const Vector4<T> &col2,
	const Vector4<T> &col3
)
{
	m_col0 = col0;
	m_col1 = col1;
	m_col2 = col2;
	m_col3 = col3;
	return *this;
}

template <typename T>
inline Columns4X4<T> &Columns4X4<T>::Initialize(
	const Vector4<T> &col0,
	const Vector4<T> &col1,
	const Vector4<T> &col2
)
{
	m_col0 = col0;
	m_col1 = col1;
	m_col2 = col2;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Columns4X4<T>::operator=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Columns4X4<T> &Columns4X4<T>::operator=(const Rows4X4<T> &rows)
{
	m_col0.Initialize(rows[0][0], rows[1][0], rows[2][0], rows[3][0]);
	m_col1.Initialize(rows[0][1], rows[1][1], rows[2][1], rows[3][1]);
	m_col2.Initialize(rows[0][2], rows[1][2], rows[2][2], rows[3][2]);
	m_col3.Initialize(rows[0][3], rows[1][3], rows[2][3], rows[3][3]);
	return *this;
}

template <typename T>
inline Columns4X4<T> &Columns4X4<T>::operator=(const Columns4X4<T> &columns)
{
	m_col0 = columns.m_col0;
	m_col1 = columns.m_col1;
	m_col2 = columns.m_col2;
	m_col3 = columns.m_col3;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math matrix backends
//////////////////////////////////////////////////////////////////////////////////////////

#include "Generic/GenericMatrixBackend.inl"

//////////////////////////////////////////////////////////////////////////////////////////
// math matrix inlines
//////////////////////////////////////////////////////////////////////////////////////////

#include "Matrix4X4.inl"
#include "AffineMatrix4X4.inl"
#include "RigidMatrix4X4.inl"
#include "SolidMatrix4X4.inl"

} // math

