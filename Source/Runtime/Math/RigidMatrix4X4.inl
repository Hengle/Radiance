// RigidMatrix4X4.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// math::operator*()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> operator*(T s, const RigidMatrix4X4<T> &m)
{
	return (m * s);
}

template <typename T>
inline Vector3<T> operator*(const Vector3<T> &v, const RigidMatrix4X4<T> &m)
{
	return m.operator*(v);
}

template <typename T>
inline Vector4<T> operator*(const Vector4<T> &v, const RigidMatrix4X4<T> &m)
{
	return m.operator*(v);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::RigidMatrix4X4()()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T>::RigidMatrix4X4() :
m_mtx()
{
}

template <typename T>
inline RigidMatrix4X4<T>::RigidMatrix4X4(const RigidMatrix4X4<T> &m) :
m_mtx(m)
{
}

template <typename T>
inline RigidMatrix4X4<T>::RigidMatrix4X4(const SolidMatrix4X4<T> &m) :
m_mtx(m)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::~RigidMatrix4X4()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T>::~RigidMatrix4X4()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Rotation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> RigidMatrix4X4<T>::Rotation(Axis3 axis, T radians)
{
	return RigidMatrix4X4<T>(AffineMatrix4X4<T>::Rotation(axis, radians));
}

template <typename T>
inline RigidMatrix4X4<T> RigidMatrix4X4<T>::Rotation(const AxisAngle<T> &axisAngle)
{
	return RigidMatrix4X4<T>(AffineMatrix4X4<T>::Rotation(axisAngle));
}

template <typename T>
inline RigidMatrix4X4<T> RigidMatrix4X4<T>::Rotation(const Quaternion<T> &rot)
{
	return RigidMatrix4X4<T>(AffineMatrix4X4<T>::Rotation(rot));
}

template <typename T>
inline RigidMatrix4X4<T> RigidMatrix4X4<T>::Rotation(const Euler<T> &euler)
{
	return RigidMatrix4X4<T>(AffineMatrix4X4<T>::Rotation(euler));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::LookAtLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> RigidMatrix4X4<T>::LookAtLH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up)
{
	RigidMatrix4X4<T> result;
	result.MakeLookAtLH(eye, at, up);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::LookAtRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> RigidMatrix4X4<T>::LookAtRH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up)
{
	RigidMatrix4X4<T> result;
	result.MakeLookAtRH(eye, at, up);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Scaling()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> RigidMatrix4X4<T>::Scaling(const T &scale)
{
	return RigidMatrix4X4<T>(AffineMatrix4X4<T>::Scaling(Scale3<T>(scale)));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Translation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> RigidMatrix4X4<T>::Translation(const Vector3<T> &offset)
{
	return RigidMatrix4X4<T>(AffineMatrix4X4<T>::Translation(offset));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::operator[]()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Vector4<T> &RigidMatrix4X4<T>::operator[](int i) const
{
	return m_mtx[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Row()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Vector4<T> &RigidMatrix4X4<T>::Row(int i) const
{
	return m_mtx.Row(i);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Column()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector4<T> RigidMatrix4X4<T>::Column(int i) const
{
	return m_mtx.Column(i);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Rows()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Rows4X4<T> &RigidMatrix4X4<T>::Rows() const
{
	return m_mtx.Rows();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Columns()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Columns4X4<T> RigidMatrix4X4<T>::Columns() const
{
	return m_mtx.Columns();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Rotation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Quaternion<T> RigidMatrix4X4<T>::Rotation() const
{
	return m_mtx.Rotation();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Inverse()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> RigidMatrix4X4<T>::Inverse() const
{
	RigidMatrix4X4<T> result = *this;
	result.Invert();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Determinant()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T RigidMatrix4X4<T>::Determinant() const
{
	return m_mtx.Determinant();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Determinant3X3()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T RigidMatrix4X4<T>::Determinant3X3() const
{
	return m_mtx.Determinant3X3();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Matrix()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Matrix4X4<T> &RigidMatrix4X4<T>::Matrix() const
{
	return m_mtx.Matrix();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Affine()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const AffineMatrix4X4<T> &RigidMatrix4X4<T>::Affine() const
{
	return m_mtx;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::MakeIdentity()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::MakeIdentity()
{
	m_mtx.MakeIdentity();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Invert()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::Invert()
{
	return Initialize(
		Vector3<T>(m_mtx[0][0], m_mtx[1][0], m_mtx[2][0]),
		Vector3<T>(m_mtx[0][1], m_mtx[1][1], m_mtx[2][1]),
		Vector3<T>(m_mtx[0][2], m_mtx[1][2], m_mtx[2][2]),
		Vector3<T>(
			-(m_mtx[0][0] * m_mtx[3][0] + m_mtx[0][1] * m_mtx[3][1] + m_mtx[0][2] * m_mtx[3][2]),
			-(m_mtx[1][0] * m_mtx[3][0] + m_mtx[1][1] * m_mtx[3][1] + m_mtx[1][2] * m_mtx[3][2]),
			-(m_mtx[2][0] * m_mtx[3][0] + m_mtx[2][1] * m_mtx[3][1] + m_mtx[2][2] * m_mtx[3][2])
		)
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::MakeRotation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::MakeRotation(Axis3 axis, T radians)
{
	m_mtx.MakeRotation(axis, radians);
	return *this;
}

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::MakeRotation(const AxisAngle<T> &axisAngle)
{
	m_mtx.MakeRotation(axisAngle);
	return *this;
}

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::MakeRotation(const Quaternion<T> &rot)
{
	m_mtx.MakeRotation(rot);
	return *this;
}

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::MakeRotation(const Euler<T> &rot)
{
	m_mtx.MakeRotation(rot);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::MakeLookAtLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::MakeLookAtLH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up)
{
	m_mtx.LookAtLH(eye, at, up);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::MakeLookAtRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::MakeLookAtRH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up)
{
	m_mtx.LookAtRH(eye, at, up);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::MakeScaling()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::MakeScaling(T scale)
{
	m_mtx.MakeScaling(Scale3<T>(scale));
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::MakeTranslation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::MakeTranslation(const Vector3<T> &offset)
{
	m_mtx.MakeTranslation(offset);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::RotateBy()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::RotateBy(Axis3 axis, T radians)
{
	m_mtx.RotateBy(axis, radians);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::ScaleBy()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::ScaleBy(T scale)
{
	m_mtx.ScaleBy(Vector3<T>(scale, scale, scale));
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::TranslateBy()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::TranslateBy(const Vector3<T> &offset)
{
	m_mtx.TranslateBy(offset);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::ScaleAndRotate()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T> RigidMatrix4X4<T>::ScaleAndRotate(const Vector3<T> &v) const
{
	return m_mtx.ScaleAndRotate(v);
}

template <typename T>
inline Vector4<T> RigidMatrix4X4<T>::ScaleAndRotate(const Vector4<T> &v) const
{
	return m_mtx.ScaleAndRotate(v);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Translate()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T> RigidMatrix4X4<T>::Translate(const Vector3<T> &v) const
{
	return m_mtx.Translate(v);
}

template <typename T>
inline Vector4<T> RigidMatrix4X4<T>::Translate(const Vector4<T> &v) const
{
	return m_mtx.Translate(v);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Transform()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T> RigidMatrix4X4<T>::Transform(const Vector3<T> &v) const
{
	return m_mtx.Transform(v);
}

template <typename T>
inline Vector4<T> RigidMatrix4X4<T>::Transform(const Vector4<T> &v) const
{
	return m_mtx.Transform(v);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::operator+()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> RigidMatrix4X4<T>::operator+(const Matrix4X4<T> &m) const
{
	return (m_mtx + m);
}

template <typename T>
inline Matrix4X4<T> RigidMatrix4X4<T>::operator+(const AffineMatrix4X4<T> &m) const
{
	return (m_mtx + m);
}

template <typename T>
inline Matrix4X4<T> RigidMatrix4X4<T>::operator+(const RigidMatrix4X4<T> &m) const
{
	return (m_mtx + m);
}

template <typename T>
inline Matrix4X4<T> RigidMatrix4X4<T>::operator+(const SolidMatrix4X4<T> &m) const
{
	return (m_mtx + m);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::operator-()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> RigidMatrix4X4<T>::operator-(const Matrix4X4<T> &m) const
{
	return (m_mtx - m);
}

template <typename T>
inline Matrix4X4<T> RigidMatrix4X4<T>::operator-(const AffineMatrix4X4<T> &m) const
{
	return (m_mtx - m);
}

template <typename T>
inline Matrix4X4<T> RigidMatrix4X4<T>::operator-(const RigidMatrix4X4<T> &m) const
{
	return (m_mtx - m);
}

template <typename T>
inline Matrix4X4<T> RigidMatrix4X4<T>::operator-(const SolidMatrix4X4<T> &m) const
{
	return (m_mtx - m);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::operator*()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> RigidMatrix4X4<T>::operator*(const Matrix4X4<T> &m) const
{
	return (m_mtx * m);
}

template <typename T>
inline AffineMatrix4X4<T> RigidMatrix4X4<T>::operator*(const AffineMatrix4X4<T> &m) const
{
	return (m_mtx * m);
}

template <typename T>
inline RigidMatrix4X4<T> RigidMatrix4X4<T>::operator*(const RigidMatrix4X4<T> &m) const
{
	return RigidMatrix4X4<T>(m_mtx * m);
}

template <typename T>
inline RigidMatrix4X4<T> RigidMatrix4X4<T>::operator*(const SolidMatrix4X4<T> &m) const
{
	return RigidMatrix4X4<T>(m_mtx * m);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::operator=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::operator=(const RigidMatrix4X4<T> &m)
{
	m_mtx = m;
	return *this;
}

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::operator=(const SolidMatrix4X4<T> &m)
{
	m_mtx = m;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::operator*=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::operator*=(const RigidMatrix4X4<T> &m)
{
	m_mtx *= m;
	return *this;
}

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::operator*=(const SolidMatrix4X4<T> &m)
{
	m_mtx *= m;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::operator*()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> RigidMatrix4X4<T>::operator*(T s) const
{
	return (m_mtx * s);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::operator/()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> RigidMatrix4X4<T>::operator/(T s) const
{
	return (m_mtx / s);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::operator*()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T> RigidMatrix4X4<T>::operator*(const Vector3<T> &v) const
{
  return (m_mtx * v);
}

template <typename T>
inline Vector4<T> RigidMatrix4X4<T>::operator*(const Vector4<T> &v) const
{
	return (m_mtx * v);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::RigidMatrix4X4()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T>::RigidMatrix4X4(const AffineMatrix4X4<T> &m) :
m_mtx(m)
{
}

template <typename T>
inline RigidMatrix4X4<T>::RigidMatrix4X4(const Rows4X4<T> &rows) :
m_mtx(rows)
{
}

template <typename T>
inline RigidMatrix4X4<T>::RigidMatrix4X4(const Columns4X4<T> &columns) :
m_mtx(columns)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::RigidMatrix4X4<T>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline RigidMatrix4X4<T> &RigidMatrix4X4<T>::Initialize(
	const Vector3<T> &row0,
	const Vector3<T> &row1,
	const Vector3<T> &row2,
	const Vector3<T> &row3
)
{
	m_mtx.Initialize(row0, row1, row2, row3);
	return *this;
}
