// AffineMatrix4X4.inl
// Inlines for AffineMatrix4X4.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// math::operator*()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> operator*(T s, const AffineMatrix4X4<T> &m)
{
	return (m * s);
}

template <typename T>
inline Vector3<T> operator*(const Vector3<T> &v, const AffineMatrix4X4<T> &m)
{
	return m.operator*(v);
}

template <typename T>
inline Vector4<T> operator*(const Vector4<T> &v, const AffineMatrix4X4<T> &m)
{
	return m.operator*(v);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::AffineMatrix4X4()()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T>::AffineMatrix4X4() :
m_mtx(Matrix4X4<T>::Identity)
{
}

template <typename T>
inline AffineMatrix4X4<T>::AffineMatrix4X4(const AffineMatrix4X4<T> &m) :
m_mtx(m)
{
}

template <typename T>
inline AffineMatrix4X4<T>::AffineMatrix4X4(const RigidMatrix4X4<T> &m) :
m_mtx(m)
{
}

template <typename T>
inline AffineMatrix4X4<T>::AffineMatrix4X4(const SolidMatrix4X4<T> &m) :
m_mtx(m)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::~AffineMatrix4X4()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T>::~AffineMatrix4X4()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::OrthoLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::OrthoLH(T width, T height, T zNear, T zFar)
{
	return AffineMatrix4X4<T>(Matrix4X4<T>::OrthoLH(width, height, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::OrthoRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::OrthoRH(T width, T height, T zNear, T zFar)
{
	return AffineMatrix4X4<T>(Matrix4X4<T>::OrthoRH(width, height, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::OrthoOffCenterLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::OrthoOffCenterLH(T left, T right, T bottom, T top, T zNear, T zFar)
{
	return AffineMatrix4X4<T>(Matrix4X4<T>::OrthoOffCenterLH(left, right, bottom, top, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::OrthoOffCenterRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::OrthoOffCenterRH(T left, T right, T bottom, T top, T zNear, T zFar)
{
	return AffineMatrix4X4<T>(Matrix4X4<T>::OrthoOffCenterRH(left, right, bottom, top, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Rotation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::Rotation(Axis3 axis, T radians)
{
	AffineMatrix4X4<T> result;
	result.MakeRotation(axis, radians);
	return result;
}

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::Rotation(const AxisAngle<T> &axisAngle)
{
	AffineMatrix4X4<T> result;
	result.MakeRotation(axisAngle);
	return result;
}

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::Rotation(const Quaternion<T> &rot)
{
	AffineMatrix4X4<T> result;
	result.MakeRotation(rot);
	return result;
}

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::Rotation(const Euler<T> &euler)
{
	AffineMatrix4X4<T> result;
	result.MakeRotation(euler);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::LookAtLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::LookAtLH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up)
{
	AffineMatrix4X4<T> result;
	result.MakeLookAtLH(eye, at, up);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::LookAtRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::LookAtRH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up)
{
	AffineMatrix4X4<T> result;
	result.MakeLookAtRH(eye, at, up);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Scaling()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::Scaling(const Scale3<T> &scale)
{
	AffineMatrix4X4<T> result;
	result.MakeScaling(scale);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Translation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::Translation(const Vector3<T> &offset)
{
	AffineMatrix4X4<T> result;
	result.MakeTranslation(offset);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Transformation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::Transformation(
	T                   scaling,
	const Vector3<T>    *rotationCenter,
	const Quaternion<T> *rotation,
	const Vector3<T>    *translation
)
{
	AffineMatrix4X4<T> result;
	result.MakeTransformation(
		scaling,
		rotationCenter,
		rotation,
		translation
	);
	return result;
}

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::Transformation(
	const Vector3<T>    *scalingCenter,
	const Quaternion<T> *scalingRotation,
	const Vector3<T>    *scaling,
	const Vector3<T>    *rotationCenter,
	const Quaternion<T> *rotation,
	const Vector3<T>    *translation
)
{
	AffineMatrix4X4<T> result;
	result.MakeTransformation(
		scalingCenter,
		scalingRotation,
		scaling,
		rotationCenter,
		rotation,
		translation
	);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Translation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::Transformation2D(
	T                scaling,
	const Vector2<T> *rotationCenter,
	T                rotation,
	const Vector2<T> *translation
)
{
	AffineMatrix4X4<T> result;
	result.MakeTransformation2D(
		scaling,
		rotationCenter,
		rotation,
		translation
	);
	return result;
}

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::Transformation2D(
	const Vector2<T> *scalingCenter,
	T                scalingRotation,
	const Vector2<T> *scaling,
	const Vector2<T> *rotationCenter,
	T                rotation,
	const Vector2<T> *translation
)
{
	AffineMatrix4X4<T> result;
	result.MakeTransformation2D(
		scalingCenter,
		scalingRotation,
		scaling,
		rotationCenter,
		rotation,
		translation
	);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::operator[]()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Vector4<T> &AffineMatrix4X4<T>::operator[](int i) const
{
	return m_mtx[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Row()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Vector4<T> &AffineMatrix4X4<T>::Row(int i) const
{
	return m_mtx.Row(i);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Column()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector4<T> AffineMatrix4X4<T>::Column(int i) const
{
	return m_mtx.Column(i);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Rows()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Rows4X4<T> &AffineMatrix4X4<T>::Rows() const
{
	return m_mtx.Rows();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Columns()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Columns4X4<T> AffineMatrix4X4<T>::Columns() const
{
	return m_mtx.Columns();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Rotation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Quaternion<T> AffineMatrix4X4<T>::Rotation() const
{
	return m_mtx.Rotation();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Inverse()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::Inverse() const
{
	AffineMatrix4X4<T> result = *this;
	result.Invert();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Determinant()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T AffineMatrix4X4<T>::Determinant() const
{
	return m_mtx.Determinant();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Determinant3X3()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T AffineMatrix4X4<T>::Determinant3X3() const
{
	return m_mtx.Determinant3X3();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Matrix()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Matrix4X4<T> &AffineMatrix4X4<T>::Matrix() const
{
	return m_mtx;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::MakeIdentity()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeIdentity()
{
	return Initialize(
		Vector3<T>(T(1.0), T(0.0), T(0.0)),
		Vector3<T>(T(0.0), T(1.0), T(0.0)),
		Vector3<T>(T(0.0), T(0.0), T(1.0)),
		Vector3<T>(T(0.0), T(0.0), T(0.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Invert()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::Invert()
{
	// A = upper left 3X3 submatrix
	// C = lower left 3 vector

	// Calculate the determinant of A.
	T detA = Determinant3X3();

	// Bail if A is nonsingular
	if (NearlyZero(detA)) { return MakeIdentity(); }

	// Calculate inverse of A = adj(A) / det(A)
	T oodetA = Reciprocal(detA);
	Vector3<T> invA[3] = {
		Vector3<T>(
			 math::Determinant(m_mtx[1][1], m_mtx[1][2], m_mtx[2][1], m_mtx[2][2]) * oodetA,
			-math::Determinant(m_mtx[0][1], m_mtx[0][2], m_mtx[2][1], m_mtx[2][2]) * oodetA,
			 math::Determinant(m_mtx[0][1], m_mtx[0][2], m_mtx[1][1], m_mtx[1][2]) * oodetA
		),
		Vector3<T>(
			-math::Determinant(m_mtx[1][0], m_mtx[1][2], m_mtx[2][0], m_mtx[2][2]) * oodetA,
			 math::Determinant(m_mtx[0][0], m_mtx[0][2], m_mtx[2][0], m_mtx[2][2]) * oodetA,
			-math::Determinant(m_mtx[0][0], m_mtx[0][2], m_mtx[1][0], m_mtx[1][2]) * oodetA
		),
		Vector3<T>(
			 math::Determinant(m_mtx[1][0], m_mtx[1][1], m_mtx[2][0], m_mtx[2][1]) * oodetA,
			-math::Determinant(m_mtx[0][0], m_mtx[0][1], m_mtx[2][0], m_mtx[2][1]) * oodetA,
			 math::Determinant(m_mtx[0][0], m_mtx[0][1], m_mtx[1][0], m_mtx[1][1]) * oodetA
		)
	};

	// result A = inverse(A) = adj(A) / det(A)
	// result C = -C * inverse(A)
	return Initialize(
		invA[0], invA[1], invA[2],
		Vector3<T>(
			-(m_mtx[3][0] * invA[0][0] + m_mtx[3][1] * invA[1][0] + m_mtx[3][2] * invA[2][0]),
			-(m_mtx[3][0] * invA[0][1] + m_mtx[3][1] * invA[1][1] + m_mtx[3][2] * invA[2][1]),
			-(m_mtx[3][0] * invA[0][2] + m_mtx[3][1] * invA[1][2] + m_mtx[3][2] * invA[2][2])
		)
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::MakeOrthoLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeOrthoLH(T width, T height, T zNear, T zFar)
{
	return operator=(OrthoLH(width, height, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::MakeOrthoRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeOrthoRH(T width, T height, T zNear, T zFar)
{
	return operator=(OrthoRH(width, height, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::MakeOrthoOffCenterLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeOrthoOffCenterLH(T left, T right, T bottom, T top, T zNear, T zFar)
{
	return operator=(OrthoOffCenterLH(left, right, bottom, top, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::MakeOrthoOffCenterRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeOrthoOffCenterRH(T left, T right, T bottom, T top, T zNear, T zFar)
{
	return operator=(OrthoOffCenterRH(left, right, bottom, top, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::MakeRotation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeRotation(Axis3 axis, T radians)
{
	T s, c;
	SinAndCos(&s, &c, radians);

	switch (axis)
	{
		case AxisX:
			return Initialize(
				Vector3<T>(T(1.0), T(0.0), T(0.0)),
				Vector3<T>(T(0.0),      c,      s),
				Vector3<T>(T(0.0),     -s,      c),
				Vector3<T>(T(0.0), T(0.0), T(0.0))
			);

		case AxisY:
			return Initialize(
				Vector3<T>(    c, T(0.0),     -s),
				Vector3<T>(T(0.0), T(1.0), T(0.0)),
				Vector3<T>(    s, T(0.0),      c),
				Vector3<T>(T(0.0), T(0.0), T(0.0))
			);

		case AxisZ:
			return Initialize(
				Vector3<T>(    c,      s, T(0.0)),
				Vector3<T>(   -s,      c, T(0.0)),
				Vector3<T>(T(0.0), T(0.0), T(1.0)),
				Vector3<T>(T(0.0), T(0.0), T(0.0))
			);

		default:
			return MakeIdentity();
  }

	return *this;
}

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeRotation(const AxisAngle<T> &axisAngle)
{
	T s, c;
	SinAndCos(&s, &c, axisAngle.Angle());
	T t = T(1.0) - c;
	const Vector3<T> &u = axisAngle.Axis();
	T tx = t * u[0];
	T txx = tx * u[0];
	T txy = tx * u[1];
	T txz = tx * u[2];
	T ty = t * u[1];
	T tyy = ty * u[1];
	T tyz = ty * u[2];
	T tzz = ty * u[2] * u[2];
	T sx = s * u[0];
	T sy = s * u[1];
	T sz = s * u[2];

	return Initialize(
		Vector3<T>(txx + c,	txy + sz, txz - sy),
		Vector3<T>(txy - sz, tyy + c, tyz + sx),
		Vector3<T>(txz + sy, tyz - sx, tzz + c),
		Vector3<T>(T(0.0), T(0.0), T(0.0))
	);
}

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeRotation(const Quaternion<T> &rot)
{
	T s = T(2.0) / rot.Magnitude();

	T xs = rot[0] * s;
	T ys = rot[1] * s;
	T zs = rot[2] * s;

	T wx = rot[3] * xs;
	T wy = rot[3] * ys;
	T wz = rot[3] * zs;

	T xx = rot[0] * xs;
	T xy = rot[0] * ys;
	T xz = rot[0] * zs;

	T yy = rot[1] * ys;
	T yz = rot[1] * zs;

	T zz = rot[2] * zs;

	return Initialize(
		Vector3<T>(T(1.0) - (yy + zz), xy + wz, xz - wy),
		Vector3<T>(xy - wz, T(1.0) - (xx + zz), yz + wx),
		Vector3<T>(xz + wy, yz - wx, T(1.0) - (xx + yy)),
		Vector3<T>(T(0.0),	T(0.0),	T(0.0))
	);
}

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeRotation(const Euler<T> &rot)
{
	T ys, yc, ps, pc, rs, rc;
	SinAndCos(&ys, &yc, rot.Yaw());
	SinAndCos(&ps, &pc, rot.Pitch());
	SinAndCos(&rs, &rc, rot.Roll());
	T rsps = rs * ps;
	T rcps = rc * ps;
	return Initialize(
		Vector3<T>(rc * yc + rsps * ys, rs * pc, -rc * ys + rsps * yc),
		Vector3<T>(-rs * yc + rcps * ys, rc * pc, rs * ys + rcps * yc),
		Vector3<T>(pc * ys, -ps, pc * yc),
		Vector3<T>(T(0.0),	T(0.0),	T(0.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::MakeLookAtLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeLookAtLH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up)
{
	m_mtx.LookAtLH(eye, at, up);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::MakeLookAtRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeLookAtRH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up)
{
	m_mtx.LookAtRH(eye, at, up);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::MakeScaling()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeScaling(const Scale3<T> &scale)
{
	return Initialize(
		Vector3<T>(scale[0],   T(0.0),   T(0.0)),
		Vector3<T>( T(0.0), scale[1],   T(0.0)),
		Vector3<T>( T(0.0),   T(0.0), scale[2]),
		Vector3<T>( T(0.0),   T(0.0),   T(0.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::MakeTranslation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeTranslation(const Vector3<T> &offset)
{
	return Initialize(
		Vector3<T>(  T(1.0),    T(0.0),    T(0.0)),
		Vector3<T>(  T(0.0),    T(1.0),    T(0.0)),
		Vector3<T>(  T(0.0),    T(0.0),    T(1.0)),
		Vector3<T>(offset[0], offset[1], offset[2])
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::MakeTransformation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeTransformation(
	T                   scaling,
	const Vector3<T>    *rotationCenter,
	const Quaternion<T> *rotation,
	const Vector3<T>    *translation
)
{
	return TraitsType::MakeTransformation(
		*this,
		scaling,
		rotationCenter,
		rotation,
		translation
	);
}

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeTransformation(
	const Vector3<T>    *scalingCenter,
	const Quaternion<T> *scalingRotation,
	const Vector3<T>    *scaling,
	const Vector3<T>    *rotationCenter,
	const Quaternion<T> *rotation,
	const Vector3<T>    *translation
)
{
	return TraitsType::MakeTransformation(
		*this,
		scalingCenter,
		scalingRotation,
		scaling,
		rotationCenter,
		rotation,
		translation
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::MakeTranslation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeTransformation2D(
	T                scaling,
	const Vector2<T> *rotationCenter,
	T                rotation,
	const Vector2<T> *translation
)
{
	return TraitsType::MakeTransformation2D(
		*this,
		scaling,
		rotationCenter,
		rotation,
		translation
	);
}

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::MakeTransformation2D(
	const Vector2<T> *scalingCenter,
	T                scalingRotation,
	const Vector2<T> *scaling,
	const Vector2<T> *rotationCenter,
	T                rotation,
	const Vector2<T> *translation
)
{
	return TraitsType::MakeTransformation2D(
		*this,
		scalingCenter,
		scalingRotation,
		scaling,
		rotationCenter,
		rotation,
		translation
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::RotateBy()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::RotateBy(Axis3 axis, T radians)
{
	RAD_ASSERT(IsValid());
	m_mtx.RotateBy(axis, radians);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::ScaleBy()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::ScaleBy(const Vector3<T> &scale)
{
	RAD_ASSERT(IsValid());
	m_mtx.ScaleBy(scale);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::TranslateBy()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::TranslateBy(const Vector3<T> &offset)
{
	RAD_ASSERT(IsValid());
	m_mtx[3].Vector3<T>::operator +=(offset);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::ScaleAndRotate()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T> AffineMatrix4X4<T>::ScaleAndRotate(const Vector3<T> &v) const
{
	return m_mtx.Transform3X3(v);
}

template <typename T>
inline Vector4<T> AffineMatrix4X4<T>::ScaleAndRotate(const Vector4<T> &v) const
{
	return m_mtx.Transform3X3(v);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Translate()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T> AffineMatrix4X4<T>::Translate(const Vector3<T> &v) const
{
	return (v + m_mtx[3]);
}

template <typename T>
inline Vector4<T> AffineMatrix4X4<T>::Translate(const Vector4<T> &v) const
{
	return Vector4<T>(v.Vector3<T>::operator+(m_mtx[3]), v[3]);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Transform()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T> AffineMatrix4X4<T>::Transform(const Vector3<T> &v) const
{
  return (*this * v);
}

template <typename T>
inline Vector4<T> AffineMatrix4X4<T>::Transform(const Vector4<T> &v) const
{
  return (*this * v);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::operator+()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> AffineMatrix4X4<T>::operator+(const Matrix4X4<T> &m) const
{
	return Matrix4X4<T>(
		Vector4<T>(m_mtx[0].Vector3<T>::operator+(m[0]), m[0][3]),
		Vector4<T>(m_mtx[1].Vector3<T>::operator+(m[1]), m[1][3]),
		Vector4<T>(m_mtx[2].Vector3<T>::operator+(m[2]), m[2][3]),
		Vector4<T>(m_mtx[3].Vector3<T>::operator+(m[3]), T(1.0) + m[3][3])
	);
}

template <typename T>
inline Matrix4X4<T> AffineMatrix4X4<T>::operator+(const AffineMatrix4X4<T> &m) const
{
	return Matrix4X4<T>(
		Vector4<T>(m_mtx[0].Vector3<T>::operator+(m[0]), T(0.0)),
		Vector4<T>(m_mtx[1].Vector3<T>::operator+(m[1]), T(0.0)),
		Vector4<T>(m_mtx[2].Vector3<T>::operator+(m[2]), T(0.0)),
		Vector4<T>(m_mtx[3].Vector3<T>::operator+(m[3]), T(2.0))
	);
}

template <typename T>
inline Matrix4X4<T> AffineMatrix4X4<T>::operator+(const RigidMatrix4X4<T> &m) const
{
	return operator+(m.m_mtx);
}

template <typename T>
inline Matrix4X4<T> AffineMatrix4X4<T>::operator+(const SolidMatrix4X4<T> &m) const
{
	return operator+(m.m_mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::operator-()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> AffineMatrix4X4<T>::operator-(const Matrix4X4<T> &m) const
{
	return Matrix4X4<T>(
		Vector4<T>(m_mtx[0].Vector3<T>::operator-(m[0]), -m[0][3]),
		Vector4<T>(m_mtx[1].Vector3<T>::operator-(m[1]), -m[1][3]),
		Vector4<T>(m_mtx[2].Vector3<T>::operator-(m[2]), -m[2][3]),
		Vector4<T>(m_mtx[3].Vector3<T>::operator-(m[3]), T(1.0) - m[3][3])
	);
}

template <typename T>
inline Matrix4X4<T> AffineMatrix4X4<T>::operator-(const AffineMatrix4X4<T> &m) const
{
	return Matrix4X4<T>(
		Vector4<T>(m_mtx[0].Vector3<T>::operator-(m[0]), T(0.0)),
		Vector4<T>(m_mtx[1].Vector3<T>::operator-(m[1]), T(0.0)),
		Vector4<T>(m_mtx[2].Vector3<T>::operator-(m[2]), T(0.0)),
		Vector4<T>(m_mtx[3].Vector3<T>::operator-(m[3]), T(0.0))
	);
}

template <typename T>
inline Matrix4X4<T> AffineMatrix4X4<T>::operator-(const RigidMatrix4X4<T> &m) const
{
	return operator-(m.m_mtx);
}

template <typename T>
inline Matrix4X4<T> AffineMatrix4X4<T>::operator-(const SolidMatrix4X4<T> &m) const
{
	return operator-(m.m_mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::operator*()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> AffineMatrix4X4<T>::operator*(const Matrix4X4<T> &m) const
{
	return Matrix4X4<T>(
		Rows4X4<T>(
			Row(0) * m,
			Row(1) * m,
			Row(2) * m,
			Row(3) * m
		)
	);
}

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::operator*(const AffineMatrix4X4<T> &m) const
{
	return AffineMatrix4X4<T>(
		Rows4X4<T>(
			Row(0) * m,
			Row(1) * m,
			Row(2) * m,
			Row(3) * m
		)
	);
}

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::operator*(const RigidMatrix4X4<T> &m) const
{
	return operator*(m.m_mtx);
}

template <typename T>
inline AffineMatrix4X4<T> AffineMatrix4X4<T>::operator*(const SolidMatrix4X4<T> &m) const
{
	return operator*(m.m_mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::operator=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::operator=(const AffineMatrix4X4<T> &m)
{
	RAD_ASSERT(m.IsValid());
	return Initialize(
		Vector3<T>(m.m_mtx[0][0], m.m_mtx[0][1], m.m_mtx[0][2]),
		Vector3<T>(m.m_mtx[1][0], m.m_mtx[1][1], m.m_mtx[1][2]),
		Vector3<T>(m.m_mtx[2][0], m.m_mtx[2][1], m.m_mtx[2][2]),
		Vector3<T>(m.m_mtx[3][0], m.m_mtx[3][1], m.m_mtx[3][2])
	);
}

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::operator=(const RigidMatrix4X4<T> &m)
{
	return operator=(m.m_mtx);
}

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::operator=(const SolidMatrix4X4<T> &m)
{
	return operator=(m.m_mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::operator*=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::operator*=(const AffineMatrix4X4<T> &m)
{
	return operator=(*this * m);
}

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::operator*=(const RigidMatrix4X4<T> &m)
{
	return operator=(*this * m);
}

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::operator*=(const SolidMatrix4X4<T> &m)
{
	return operator=(*this * m);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::operator*()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> AffineMatrix4X4<T>::operator*(T s) const
{
	return Matrix4X4<T>(
		Vector4<T>(m_mtx[0][0] * s, m_mtx[0][1] * s, m_mtx[0][2] * s, T(0.0)),
		Vector4<T>(m_mtx[1][0] * s, m_mtx[1][1] * s, m_mtx[1][2] * s, T(0.0)),
		Vector4<T>(m_mtx[2][0] * s, m_mtx[2][1] * s, m_mtx[2][2] * s, T(0.0)),
		Vector4<T>(m_mtx[3][0] * s, m_mtx[3][1] * s, m_mtx[3][2] * s, s)
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::operator/()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> AffineMatrix4X4<T>::operator/(T s) const
{
	return operator*(Reciprocal(s));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::operator*()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T> AffineMatrix4X4<T>::operator*(const Vector3<T> &v) const
{
  return Vector3<T>(
		(m_mtx[0][0] * v[0] + m_mtx[1][0] * v[1] + m_mtx[2][0] * v[2] + m_mtx[3][0]),
		(m_mtx[0][1] * v[0] + m_mtx[1][1] * v[1] + m_mtx[2][1] * v[2] + m_mtx[3][1]),
		(m_mtx[0][2] * v[0] + m_mtx[1][2] * v[1] + m_mtx[2][2] * v[2] + m_mtx[3][2])
	);
}

template <typename T>
inline Vector4<T> AffineMatrix4X4<T>::operator*(const Vector4<T> &v) const
{
	return Vector4<T>(
		(m_mtx[0][0] * v[0] + m_mtx[1][0] * v[1] + m_mtx[2][0] * v[2] + m_mtx[3][0] * v[3]),
		(m_mtx[0][1] * v[0] + m_mtx[1][1] * v[1] + m_mtx[2][1] * v[2] + m_mtx[3][1] * v[3]),
		(m_mtx[0][2] * v[0] + m_mtx[1][2] * v[1] + m_mtx[2][2] * v[2] + m_mtx[3][2] * v[3]),
		v[3]
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::AffineMatrix4X4()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T>::AffineMatrix4X4(const Matrix4X4<T> &m) :
m_mtx(m)
{
}

template <typename T>
inline AffineMatrix4X4<T>::AffineMatrix4X4(const Rows4X4<T> &rows) :
m_mtx(rows)
{
}

template <typename T>
inline AffineMatrix4X4<T>::AffineMatrix4X4(const Columns4X4<T> &columns) :
m_mtx(columns)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AffineMatrix4X4<T> &AffineMatrix4X4<T>::Initialize(
	const Vector3<T> &row0,
	const Vector3<T> &row1,
	const Vector3<T> &row2,
	const Vector3<T> &row3
)
{
	RAD_ASSERT(IsValid());
	m_mtx.Initialize(row0, row1, row2, row3);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::AffineMatrix4X4<T>::IsValid()
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_DEBUG)

template <typename T>
inline bool AffineMatrix4X4<T>::IsValid() const
{
	return (
		m_mtx[0][3] == T(0.0) &&
		m_mtx[1][3] == T(0.0) &&
		m_mtx[2][3] == T(0.0) &&
		m_mtx[3][3] == T(1.0)
	);
}

#endif
