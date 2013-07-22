// Matrix4X4.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// math::operator*()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> operator*(T s, const Matrix4X4<T> &m)
{
	return (m * s);
}

template <typename T>
inline Vector3<T> operator*(const Vector3<T> &v, const Matrix4X4<T> &m)
{
	return m.operator*(v);
}

template <typename T>
inline Vector4<T> operator*(const Vector4<T> &v, const Matrix4X4<T> &m)
{
	return m.operator*(v);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Identity
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
const Matrix4X4<T> Matrix4X4<T>::Identity = Matrix4X4<T>(
	Rows4X4<T>(
		Vector4<T>(T(1.0), T(0.0), T(0.0), T(0.0)),
		Vector4<T>(T(0.0), T(1.0), T(0.0), T(0.0)),
		Vector4<T>(T(0.0), T(0.0), T(1.0), T(0.0)),
		Vector4<T>(T(0.0), T(0.0), T(0.0), T(1.0))
	)
);

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Zero
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
const Matrix4X4<T> Matrix4X4<T>::Zero = Matrix4X4<T>(
	Rows4X4<T>(
		Vector4<T>(T(0.0), T(0.0), T(0.0), T(0.0)),
		Vector4<T>(T(0.0), T(0.0), T(0.0), T(0.0)),
		Vector4<T>(T(0.0), T(0.0), T(0.0), T(0.0)),
		Vector4<T>(T(0.0), T(0.0), T(0.0), T(0.0))
	)
);

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Matrix4X4()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T>::Matrix4X4() :
m_mtx()
{
}

template <typename T>
inline Matrix4X4<T>::Matrix4X4(const Matrix4X4<T> &m) :
m_mtx(m.Rows())
{
}

template <typename T>
inline Matrix4X4<T>::Matrix4X4(const AffineMatrix4X4<T> &m) :
m_mtx(m.Rows())
{
}

template <typename T>
inline Matrix4X4<T>::Matrix4X4(const RigidMatrix4X4<T> &m) :
m_mtx(m.Rows())
{
}

template <typename T>
inline Matrix4X4<T>::Matrix4X4(const SolidMatrix4X4<T> &m) :
m_mtx(m.Rows())
{
}

template <typename T>
inline Matrix4X4<T>::Matrix4X4(const Rows4X4<T> &rows) :
m_mtx(rows)
{
}

template <typename T>
inline Matrix4X4<T>::Matrix4X4(const Columns4X4<T> &columns) :
m_mtx(columns)
{
}

template <typename T>
inline Matrix4X4<T>::Matrix4X4(
	const Vector4<T> &row0,
	const Vector4<T> &row1,
	const Vector4<T> &row2,
	const Vector4<T> &row3
) :
m_mtx(row0, row1, row2, row3)
{
}

template <typename T>
inline Matrix4X4<T>::Matrix4X4(
	const Vector3<T> &row0,
	const Vector3<T> &row1,
	const Vector3<T> &row2,
	const Vector3<T> &row3
) :
m_mtx(
	Vector4<T>(row0, 0.0f),
	Vector4<T>(row1, 0.0f),
	Vector4<T>(row2, 0.0f),
	Vector4<T>(row3, 1.0f)
)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::~Matrix4X4()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T>::~Matrix4X4()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Ortho()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Ortho(T left, T right, T bottom, T top, T zNear, T zFar)
{
	const T XSCALE = T(2.0) / (right - left);
	const T XBIAS  = (right + left) / (right - left);
	const T YSCALE = T(2.0) / (top - bottom);
	const T YBIAS  = (top + bottom) / (top - bottom);
	RAD_ASSERT(zNear != zFar);
	const T ZSCALE = T(-2.0) / (zFar - zNear);
	const T ZBIAS  = (zFar + zNear) / (zFar - zNear);
	return Matrix4X4<T>(
		Vector4<T>(XSCALE, T(0.0), T(0.0), T(0.0)),
		Vector4<T>(T(0.0), YSCALE, T(0.0), T(0.0)),
		Vector4<T>(T(0.0), T(0.0), ZSCALE, T(0.0)),
		Vector4<T>(-XBIAS, -YBIAS, -ZBIAS, T(1.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::PerspectiveFovLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::PerspectiveFovLH(T fovY, T aspect, T zNear, T zFar)
{
	const T TANFOVY = Tan(fovY / T(2.0));
	RAD_ASSERT(T(0.0) != TANFOVY);
	const T YSCALE = T(1.0) / TANFOVY;
	RAD_ASSERT(T(0.0) != aspect);
	const T XSCALE = YSCALE / aspect;
	RAD_ASSERT(zNear != zFar);
	const T ZSCALE = zFar / (zFar - zNear);
	const T ZBIAS  = -zNear * zFar / (zFar - zNear);
	return Matrix4X4<T>(
		Vector4<T>(XSCALE, T(0.0), T(0.0), T(0.0)),
		Vector4<T>(T(0.0), YSCALE, T(0.0), T(0.0)),
		Vector4<T>(T(0.0), T(0.0), ZSCALE, T(1.0)),
		Vector4<T>(T(0.0), T(0.0),  ZBIAS, T(0.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::PerspectiveFovRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::PerspectiveFovRH(T fovY, T aspect, T zNear, T zFar)
{
	const T TANFOVY = Tan(fovY / T(2.0));
	RAD_ASSERT(T(0.0) != TANFOVY);
	const T YSCALE = T(1.0) / TANFOVY;
	RAD_ASSERT(T(0.0) != aspect);
	const T XSCALE = YSCALE / aspect;
	const T ZSCALE = zFar / (zNear - zFar);
	const T ZBIAS  = zNear * zFar / (zNear - zFar);
	return Matrix4X4<T>(
		Vector4<T>(XSCALE, T(0.0), T(0.0),  T(0.0)),
		Vector4<T>(T(0.0), YSCALE, T(0.0),  T(0.0)),
		Vector4<T>(T(0.0), T(0.0), ZSCALE, T(-1.0)),
		Vector4<T>(T(0.0), T(0.0),  ZBIAS,  T(0.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::PerspectiveLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::PerspectiveLH(T width, T height, T zNear, T zFar)
{
	RAD_ASSERT(T(0.0) != width);
	const T XSCALE = T(2.0) * zNear / width;
	RAD_ASSERT(T(0.0) != height);
	const T YSCALE = T(2.0) * zNear / height;
	const T ZSCALE = zFar / (zFar - zNear);
	const T ZBIAS  = zNear * zFar / (zNear - zFar);
	return Matrix4X4<T>(
		Vector4<T>(XSCALE, T(0.0), T(0.0), T(0.0)),
		Vector4<T>(T(0.0), YSCALE, T(0.0), T(0.0)),
		Vector4<T>(T(0.0), T(0.0), ZSCALE, T(1.0)),
		Vector4<T>(T(0.0), T(0.0),  ZBIAS, T(0.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::PerspectiveRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::PerspectiveRH(T width, T height, T zNear, T zFar)
{
	RAD_ASSERT(T(0.0) != width);
	const T XSCALE = T(2.0) * zNear / width;
	RAD_ASSERT(T(0.0) != height);
	const T YSCALE = T(2.0) * zNear / height;
	const T ZSCALE = zFar / (zNear - zFar);
	const T ZBIAS  = zNear * zFar / (zNear - zFar);
	return Matrix4X4<T>(
		Vector4<T>(XSCALE, T(0.0), T(0.0),  T(0.0)),
		Vector4<T>(T(0.0), YSCALE, T(0.0),  T(0.0)),
		Vector4<T>(T(0.0), T(0.0), ZSCALE, T(-1.0)),
		Vector4<T>(T(0.0), T(0.0),  ZBIAS,  T(0.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::PerspectiveOffCenterLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::PerspectiveOffCenterLH(T left, T right, T bottom, T top, T zNear, T zFar)
{
	RAD_ASSERT(left != right);
	const T XSCALE = T(2.0) * zNear / (right - left);
	RAD_ASSERT(top != bottom);
	const T YSCALE = T(2.0) * zNear / (top - bottom);
	const T ZSCALE = zFar / (zFar - zNear);
	const T ZBIAS  = zNear * zFar / (zNear - zFar);
	const T XSKEW  = (left + right) / (left - right);
	const T YSKEW  = (top + bottom) / (bottom - top);
	return Matrix4X4<T>(
		Vector4<T>(XSCALE, T(0.0), T(0.0), T(0.0)),
		Vector4<T>(T(0.0), YSCALE, T(0.0), T(0.0)),
		Vector4<T>( XSKEW,  YSKEW, ZSCALE, T(1.0)),
		Vector4<T>(T(0.0), T(0.0),  ZBIAS, T(0.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::PerspectiveOffCenterRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::PerspectiveOffCenterRH(T left, T right, T bottom, T top, T zNear, T zFar)
{
	RAD_ASSERT(left != right);
	const T XSCALE = (T(2.0) * zNear) / (right - left);
	RAD_ASSERT(top != bottom);
	const T YSCALE = (T(2.0) * zNear) / (top - bottom);
	const T ZSCALE = (zFar + zNear) / (zFar - zNear);
	const T ZBIAS  = (T(2.0) * zNear * zFar) / (zFar - zNear);
	const T XSKEW  = (right + left) / (right - left);
	const T YSKEW  = (top + bottom) / (top - bottom);
	return Matrix4X4<T>(
		Vector4<T>(XSCALE, T(0.0), T(0.0),  T(0.0)),
		Vector4<T>(T(0.0), YSCALE, T(0.0),  T(0.0)),
		Vector4<T>( XSKEW,  YSKEW, -ZSCALE, T(-1.0)),
		Vector4<T>(T(0.0), T(0.0), -ZBIAS,  T(0.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Rotation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Rotation(Axis3 axis, T radians)
{
	T s, c;
	SinAndCos(&s, &c, radians);
	switch (axis)
	{
		case AxisX:
			return Matrix4X4<T>(
				Vector4<T>(T(1.0), T(0.0), T(0.0), T(0.0)),
				Vector4<T>(T(0.0),      c,      s, T(0.0)),
				Vector4<T>(T(0.0),     -s,      c, T(0.0)),
				Vector4<T>(T(0.0), T(0.0), T(0.0), T(1.0))
			);

		case AxisY:
			return Matrix4X4<T>(
				Vector4<T>(    c, T(0.0),     -s, T(0.0)),
				Vector4<T>(T(0.0), T(1.0), T(0.0), T(0.0)),
				Vector4<T>(    s, T(0.0),      c, T(0.0)),
				Vector4<T>(T(0.0), T(0.0), T(0.0), T(1.0))
			);

		case AxisZ:
			return Matrix4X4<T>(
				Vector4<T>(    c,      s, T(0.0), T(0.0)),
				Vector4<T>(   -s,      c, T(0.0), T(0.0)),
				Vector4<T>(T(0.0), T(0.0), T(1.0), T(0.0)),
				Vector4<T>(T(0.0), T(0.0), T(0.0), T(1.0))
			);

		default:
			return Identity;
	}
}

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Rotation(const AxisAngle<T> &axisAngle)
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

	return Matrix4X4<T>(
		Vector4<T>(txx + c,	txy + sz, txz - sy,	T(0.0)),
		Vector4<T>(txy - sz, tyy + c, tyz + sx,	T(0.0)),
		Vector4<T>(txz + sy, tyz - sx, tzz + c, T(0.0)),
		Vector4<T>(T(0.0), T(0.0), T(0.0), T(1.0))
	);
}

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Rotation(const Quaternion<T> &rot)
{
	T xs = rot[0]*2;
	T ys = rot[1]*2;
	T zs = rot[2]*2;

	T wx = rot[3] * xs;
	T wy = rot[3] * ys;
	T wz = rot[3] * zs;

	T xx = rot[0] * xs;
	T xy = rot[0] * ys;
	T xz = rot[0] * zs;

	T yy = rot[1] * ys;
	T yz = rot[1] * zs;
	T zz = rot[2] * zs;

	return Matrix4X4<T>(
		Vector4<T>(T(1.0) - (yy + zz),	xy + wz, xz - wy,	T(0.0)),
		Vector4<T>(xy - wz, T(1.0) - (xx + zz), yz + wx,	T(0.0)),
		Vector4<T>(xz + wy, yz - wx,	T(1.0) - (xx + yy),	T(0.0)),
		Vector4<T>(T(0.0),	T(0.0),	T(0.0),	T(1.0))
	);
}

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Rotation(const Euler<T> &euler)
{
	T ys, yc, ps, pc, rs, rc;
	SinAndCos(&ys, &yc, euler.Yaw());
	SinAndCos(&ps, &pc, euler.Pitch());
	SinAndCos(&rs, &rc, euler.Roll());
	T rsps = rs * ps;
	T rcps = rc * ps;

	return Matrix4X4<T>(
		Vector4<T>( rc * yc + rsps * ys, rs * pc, -rc * ys + rsps * yc,	T(0.0)),
		Vector4<T>(-rs * yc + rcps * ys, rc * pc,  rs * ys + rcps * yc,	T(0.0)),
		Vector4<T>(             pc * ys,     -ps,              pc * yc, T(0.0)),
		Vector4<T>(              T(0.0),  T(0.0),               T(0.0), T(1.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Angles()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Vector3<T> Matrix4X4<T>::Angles() const
{
	T rx = math::ArcTan2(m_mtx[1][2], m_mtx[2][2]);
	T c2 = math::SquareRoot((m_mtx[0][0] * m_mtx[0][0]) + (m_mtx[0][1] * m_mtx[0][1]));
	T ry = math::ArcTan2(-m_mtx[0][2], c2);

	T s1, c1;
	math::SinAndCos(&s1, &c1, rx);

	T rz = math::ArcTan2(
		(s1*m_mtx[2][0]) - (c1*m_mtx[1][0]), 
		(c1*m_mtx[1][1]) - (s1*m_mtx[2][1])
	);
	
	return Vector3<T>(rx, ry, rz);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::LookAtLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::LookAtLH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up)
{
	return LookAt(eye, at - eye, up);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::LookAtRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::LookAtRH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up)
{
	return LookAt(eye, eye - at, up);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Scaling()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Scaling(const Scale3<T> &scale)
{
	return Matrix4X4<T>(
		Vector4<T>(scale[0],   T(0.0),   T(0.0),   T(0.0)),
		Vector4<T>(  T(0.0), scale[1],   T(0.0),   T(0.0)),
		Vector4<T>(  T(0.0),   T(0.0), scale[2],   T(0.0)),
		Vector4<T>(  T(0.0),   T(0.0),   T(0.0),   T(1.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Translation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Translation(const Vector3<T> &offset)
{
	return Matrix4X4<T>(
		Vector4<T>(  T(1.0),    T(0.0),    T(0.0),    T(0.0)),
		Vector4<T>(  T(0.0),    T(1.0),    T(0.0),    T(0.0)),
		Vector4<T>(  T(0.0),    T(0.0),    T(1.0),    T(0.0)),
		Vector4<T>(offset[0], offset[1], offset[2],    T(1.0))
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Transformation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Transformation(
	T                   scaling,
	const Vector3<T>    *rotationCenter,
	const Quaternion<T> *rotation,
	const Vector3<T>    *translation
)
{
	Matrix4X4<T> result;
	result.MakeTransformation(
		scaling,
		rotationCenter,
		rotation,
		translation
	);
	return result;
}

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Transformation(
	const Vector3<T>    *scalingCenter,
	const Quaternion<T> *scalingRotation,
	const Vector3<T>    *scaling,
	const Vector3<T>    *rotationCenter,
	const Quaternion<T> *rotation,
	const Vector3<T>    *translation
)
{
	Matrix4X4<T> result;
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
// math::Matrix4X4<T>::Translation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Transformation2D(
	T                scaling,
	const Vector2<T> *rotationCenter,
	T                rotation,
	const Vector2<T> *translation
)
{
	Matrix4X4<T> result;
	result.MakeTransformation2D(
		scaling,
		rotationCenter,
		rotation,
		translation
	);
	return result;
}

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Transformation2D(
	const Vector2<T> *scalingCenter,
	T                scalingRotation,
	const Vector2<T> *scaling,
	const Vector2<T> *rotationCenter,
	T                rotation,
	const Vector2<T> *translation
)
{
	Matrix4X4<T> result;
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
// math::Matrix4X4<T>::operator[]()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Vector4<T> &Matrix4X4<T>::operator[](int i) const
{
	return m_mtx[i];
}

template <typename T>
inline Vector4<T> &Matrix4X4<T>::operator[](int i)
{
	return m_mtx[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Row()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Vector4<T> &Matrix4X4<T>::Row(int i) const
{
	return m_mtx.Row(i);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Column()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector4<T> Matrix4X4<T>::Column(int i) const
{
	return m_mtx.Column(i);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Rows()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const Rows4X4<T> &Matrix4X4<T>::Rows() const
{
	return m_mtx;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Columns()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Columns4X4<T> Matrix4X4<T>::Columns() const
{
	return Columns4X4<T>(m_mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Rotation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Quaternion<T> Matrix4X4<T>::Rotation() const
{
	Quaternion<T> res;
	T diag = m_mtx[0][0] + m_mtx[1][1] + m_mtx[2][2] + T(1);
	if (diag <= T(0))
	{
		static const int next[3] = { 1, 2, 0 };
		int i = 0;
		if (m_mtx[1][1] > m_mtx[0][0]) { i = 1; }
		if (m_mtx[2][2] > m_mtx[i][i]) { i = 2; }
		int j = next[i];
		int k = next[j];
		T s = SquareRoot(m_mtx[i][i] - m_mtx[j][j] - m_mtx[k][k] + T(1.0)) * T(2);
		T oos = T(1) / s;
		res[i] = T(0.5) / s;
		res[j] = (m_mtx[i][j] + m_mtx[j][i]) * oos;
		res[k] = (m_mtx[i][k] + m_mtx[k][i]) * oos;
		res[3] = (m_mtx[j][k] - m_mtx[k][j]) * oos;
	}
	else
	{
		T s = SquareRoot(diag);
		T oos = T(0.5) / s;
		res[0] = (m_mtx[1][2] - m_mtx[2][1]) * oos;
		res[1] = (m_mtx[2][0] - m_mtx[0][2]) * oos;
		res[2] = (m_mtx[0][1] - m_mtx[1][0]) * oos;
		res[3] = s * T(0.5);
	}
	return res;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Transpose()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::Transpose()
{
	return operator=(Transposed());
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Inverse()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Inverted() const
{
	Matrix4X4<T> result = *this;
	result.Invert();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Transposed()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::Transposed() const
{
	return Matrix4X4<T>(Column(0), Column(1), Column(2), Column(3));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Determinant()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T Matrix4X4<T>::Determinant() const
{
	return math::Determinant(
		m_mtx[0][0], m_mtx[0][1], m_mtx[0][2], m_mtx[0][3],
		m_mtx[1][0], m_mtx[1][1], m_mtx[1][2], m_mtx[1][3],
		m_mtx[2][0], m_mtx[2][1], m_mtx[2][2], m_mtx[2][3],
		m_mtx[3][0], m_mtx[3][1], m_mtx[3][2], m_mtx[3][3]
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Determinant3X3()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T Matrix4X4<T>::Determinant3X3() const
{
	return math::Determinant(
		m_mtx[0][0], m_mtx[0][1], m_mtx[0][2],
		m_mtx[1][0], m_mtx[1][1], m_mtx[1][2],
		m_mtx[2][0], m_mtx[2][1], m_mtx[2][2]
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::Initialize(
	const Vector4<T> &row0,
	const Vector4<T> &row1,
	const Vector4<T> &row2,
	const Vector4<T> &row3
)
{
	m_mtx.Initialize(row0, row1, row2, row3);
	return *this;
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::Initialize(
	const Vector3<T> &row0,
	const Vector3<T> &row1,
	const Vector3<T> &row2,
	const Vector3<T> &row3
)
{
	m_mtx.Initialize(row0, row1, row2, row3);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakeIdentity()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeIdentity()
{
	return operator=(Identity);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Invert()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Matrix4X4<T> &Matrix4X4<T>::Invert()
{
	Matrix4X4 mtx(*this);
	int col;

	// Make this the identity matrix

	MakeIdentity();

	// Go through columns

	for (col = 0; col < 4; col++)
	{
		int row, col2;
		int rowMax = col;
		T t;

		// Find the row with max value in this column

		for (row = col + 1; row < 4; row++)
		{
			if (Abs(mtx[row][col]) > Abs(mtx[rowMax][col]))	{ rowMax = row;	}
		}

		// If the max value here is 0, we can't invert.  Return identity.

		if (mtx[rowMax][col] == T(0.0)) { return MakeIdentity(); };

		// Swap row "rowMax" with row "col"

		for (col2 = 0; col2 < 4; col2++)
		{
			std::swap(m_mtx[col][col2], m_mtx[rowMax][col2]);
			std::swap(mtx[col][col2], mtx[rowMax][col2]);
		}

		// Now everything we do is on row "col".
		// Set the max cell to 1 by dividing the entire row by that value

		t = Reciprocal(mtx[col][col]);
		for (col2 = 0; col2 < 4; col2++)
		{
			m_mtx[col][col2] *= t;
			mtx[col][col2] *= t;
		}

		// Now do the other rows, so that this column only has a 1 and 0's

		for (row = 0; row < 4; row++)
		{
			if (row != col)
			{
				t = mtx[row][col];
				for (col2 = 0; col2 < 4; col2++)
				{
					mtx[row][col2] -= mtx[col][col2] * t;
					m_mtx[row][col2] -= m_mtx[col][col2] * t;
				}
			}
		}
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakeOrthoLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeOrthoLH(T width, T height, T zNear, T zFar)
{
	return operator=(OrthoLH(width, height, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakeOrthoRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeOrthoRH(T width, T height, T zNear, T zFar)
{
	return operator=(OrthoRH(width, height, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakeOrthoOffCenterLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeOrthoOffCenterLH(T left, T right, T bottom, T top, T zNear, T zFar)
{
	return operator=(OrthoOffCenterLH(left, right, bottom, top, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakeOrthoOffCenterRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeOrthoOffCenterRH(T left, T right, T bottom, T top, T zNear, T zFar)
{
	return operator=(OrthoOffCenterRH(left, right, bottom, top, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakePerspectiveFovLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakePerspectiveFovLH(T fovY, T aspect, T zNear, T zFar)
{
	return operator=(PerspectiveFovLH(fovY, aspect, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakePerspectiveFovRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakePerspectiveFovRH(T fovY, T aspect, T zNear, T zFar)
{
	return operator=(PerspectiveFovRH(fovY, aspect, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakePerspectiveLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakePerspectiveLH(T width, T height, T zNear, T zFar)
{
	return operator=(MakePerspectiveLH(width, height, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakePerspectiveRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakePerspectiveRH(T width, T height, T zNear, T zFar)
{
	return operator=(MakePerspectiveRH(width, height, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakePerspectiveOffCenterLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakePerspectiveOffCenterLH(T left, T right, T bottom, T top, T zNear, T zFar)
{
	return operator=(MakePerspectiveOffCenterLH(left, right, bottom, top, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakePerspectiveOffCenterRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakePerspectiveOffCenterRH(T left, T right, T bottom, T top, T zNear, T zFar)
{
	return operator=(MakePerspectiveOffCenterRH(left, right, bottom, top, zNear, zFar));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakeRotation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeRotation(Axis3 axis, T radians)
{
	return operator=(Rotation(axis, radians));
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeRotation(const AxisAngle<T> &axisAngle)
{
	return operator=(Rotation(axisAngle));
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeRotation(const Quaternion<T> &rot)
{
	return operator=(Rotation(rot));
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeRotation(const Euler<T> &euler)
{
	return operator=(Rotation(euler));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakeLookAtLH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeLookAtLH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up)
{
	return operator=(LookAtLH(eye, at, up));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakeLookAtRH()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeLookAtRH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up)
{
	return operator=(LookAtRH(eye, at, up));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakeScaling()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeScaling(const Scale3<T> &scale)
{
	return operator=(Scaling(scale));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakeTranslation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeTranslation(const Vector3<T> &offset)
{
	return operator=(Translation(offset));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::MakeTransformation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeTransformation(
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
inline Matrix4X4<T> &Matrix4X4<T>::MakeTransformation(
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
// math::Matrix4X4<T>::MakeTranslation()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::MakeTransformation2D(
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
inline Matrix4X4<T> &Matrix4X4<T>::MakeTransformation2D(
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
// math::Matrix4X4<T>::RotateBy()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::RotateBy(Axis3 axis, T radians)
{
	T s, c;
	SinAndCos(&s, &c, radians);
	T t1, t2;

	switch (axis)
	{
	case AxisX:
		t1 = m_mtx[0][1];
		t2 = m_mtx[0][2];
		m_mtx[0][1] = t1 * c - t2 * s;
		m_mtx[0][2] = t1 * s + t2 * c;

		t1 = m_mtx[1][1];
		t2 = m_mtx[1][2];
		m_mtx[1][1] = t1 * c - t2 * s;
		m_mtx[1][2] = t1 * s + t2 * c;

		t1 = m_mtx[2][1];
		t2 = m_mtx[2][2];
		m_mtx[2][1] = t1 * c - t2 * s;
		m_mtx[2][2] = t1 * s + t2 * c;

		t1 = m_mtx[3][1];
		t2 = m_mtx[3][2];
		m_mtx[3][1] = t1 * c - t2 * s;
		m_mtx[3][2] = t1 * s + t2 * c;
		break;

	case AxisY:
		t1 = m_mtx[0][0];
		t2 = m_mtx[0][2];
		m_mtx[0][0] = t1 * c + t2 * s;
		m_mtx[0][2] = t2 * c - t1 * s;

		t1 = m_mtx[1][0];
		t2 = m_mtx[1][2];
		m_mtx[1][0] = t1 * c + t2 * s;
		m_mtx[1][2] = t2 * c - t1 * s;

		t1 = m_mtx[2][0];
		t2 = m_mtx[2][2];
		m_mtx[2][0] = t1 * c + t2 * s;
		m_mtx[2][2] = t2 * c - t1 * s;

		t1 = m_mtx[3][0];
		t2 = m_mtx[3][2];
		m_mtx[3][0] = t1 * c + t2 * s;
		m_mtx[3][2] = t2 * c - t1 * s;
		break;

	case AxisZ:
		t1 = m_mtx[0][0];
		t2 = m_mtx[0][1];
		m_mtx[0][0] = t1 * c - t2 * s;
		m_mtx[0][1] = t1 * s + t2 * c;

		t1 = m_mtx[1][0];
		t2 = m_mtx[1][1];
		m_mtx[1][0] = t1 * c - t2 * s;
		m_mtx[1][1] = t1 * s + t2 * c;

		t1 = m_mtx[2][0];
		t2 = m_mtx[2][1];
		m_mtx[2][0] = t1 * c - t2 * s;
		m_mtx[2][1] = t1 * s + t2 * c;

		t1 = m_mtx[3][0];
		t2 = m_mtx[3][1];
		m_mtx[3][0] = t1 * c - t2 * s;
		m_mtx[3][1] = t1 * s + t2 * c;
		break;
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::ScaleBy()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::ScaleBy(const Vector3<T> &scale)
{
//#pragma message("MPS: fix this so it's optimized")
	return operator *=(Scaling(Scale3<T>(scale)));
	/*return Initialize(
		scale.Scale(m_mtx[0]),
		scale.Scale(m_mtx[1]),
		scale.Scale(m_mtx[2]),
		scale.Scale(m_mtx[3])
	);*/
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::TranslateBy()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Matrix4X4<T> &Matrix4X4<T>::TranslateBy(const Vector3<T> &offset)
{
	m_mtx[0][0] += m_mtx[0][3] * offset[0];
	m_mtx[0][1] += m_mtx[0][3] * offset[1];
	m_mtx[0][2] += m_mtx[0][3] * offset[2];

	m_mtx[1][0] += m_mtx[1][3] * offset[0];
	m_mtx[1][1] += m_mtx[1][3] * offset[1];
	m_mtx[1][2] += m_mtx[1][3] * offset[2];

	m_mtx[2][0] += m_mtx[2][3] * offset[0];
	m_mtx[2][1] += m_mtx[2][3] * offset[1];
	m_mtx[2][2] += m_mtx[2][3] * offset[2];

	m_mtx[3][0] += m_mtx[3][3] * offset[0];
	m_mtx[3][1] += m_mtx[3][3] * offset[1];
	m_mtx[3][2] += m_mtx[3][3] * offset[2];

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Transform()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T> Matrix4X4<T>::Transform(const Vector3<T> &v) const
{
	return (*this * v);
}

template <typename T>
inline Vector4<T> Matrix4X4<T>::Transform(const Vector4<T> &v) const
{
	return (*this * v);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Transform3X3()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T> Matrix4X4<T>::Transform3X3(const Vector3<T> &v) const
{
	return Vector3<T>(
		m_mtx[0][0] * v[0] + m_mtx[1][0] * v[1] + m_mtx[2][0] * v[2],
		m_mtx[0][1] * v[0] + m_mtx[1][1] * v[1] + m_mtx[2][1] * v[2],
		m_mtx[0][2] * v[0] + m_mtx[1][2] * v[1] + m_mtx[2][2] * v[2]
	);
}

template <typename T>
inline Vector4<T> Matrix4X4<T>::Transform3X3(const Vector4<T> &v) const
{
	return Vector4<T>(
		m_mtx[0][0] * v[0] + m_mtx[1][0] * v[1] + m_mtx[2][0] * v[2],
		m_mtx[0][1] * v[0] + m_mtx[1][1] * v[1] + m_mtx[2][1] * v[2],
		m_mtx[0][2] * v[0] + m_mtx[1][2] * v[1] + m_mtx[2][2] * v[2],
		v[3]
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::Transform4X3()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T> Matrix4X4<T>::Transform4X3(const Vector3<T> &v) const
{
	return Vector3<T>(
		m_mtx[0][0] * v[0] + m_mtx[1][0] * v[1] + m_mtx[2][0] * v[2] + m_mtx[3][0],
		m_mtx[0][1] * v[0] + m_mtx[1][1] * v[1] + m_mtx[2][1] * v[2] + m_mtx[3][1],
		m_mtx[0][2] * v[0] + m_mtx[1][2] * v[1] + m_mtx[2][2] * v[2] + m_mtx[3][2]
	);
}

template <typename T>
inline Vector4<T> Matrix4X4<T>::Transform4X3(const Vector4<T> &v) const
{
	return Vector4<T>(
		m_mtx[0][0] * v[0] + m_mtx[1][0] * v[1] + m_mtx[2][0] * v[2] + m_mtx[3][0] * v[3],
		m_mtx[0][1] * v[0] + m_mtx[1][1] * v[1] + m_mtx[2][1] * v[2] + m_mtx[3][1] * v[3],
		m_mtx[0][2] * v[0] + m_mtx[1][2] * v[1] + m_mtx[2][2] * v[2] + m_mtx[3][2] * v[3],
		v[3]
	);
}


//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::operator+()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::operator+(const Matrix4X4<T> &m) const
{
	return Matrix4X4<T>(
		m_mtx[0] + m.m_mtx[0],
		m_mtx[1] + m.m_mtx[1],
		m_mtx[2] + m.m_mtx[2],
		m_mtx[3] + m.m_mtx[3]
	);
}

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::operator+(const AffineMatrix4X4<T> &m) const
{
	return Matrix4X4<T>(
		Vector4<T>(m_mtx[0].Vector3<T>::operator+(m[0]), m_mtx[0][3]),
		Vector4<T>(m_mtx[1].Vector3<T>::operator+(m[1]), m_mtx[1][3]),
		Vector4<T>(m_mtx[2].Vector3<T>::operator+(m[2]), m_mtx[2][3]),
		m_mtx[3] + m[3]
	);
}

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::operator+(const RigidMatrix4X4<T> &m) const
{
	return operator+(m.m_mtx);
}

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::operator+(const SolidMatrix4X4<T> &m) const
{
	return operator+(m.m_mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::operator-()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::operator-(const Matrix4X4<T> &m) const
{
	return Matrix4X4<T>(
		m_mtx[0] - m.m_mtx[0],
		m_mtx[1] - m.m_mtx[1],
		m_mtx[2] - m.m_mtx[2],
		m_mtx[3] - m.m_mtx[3]
	);
}

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::operator-(const AffineMatrix4X4<T> &m) const
{
	return Matrix4X4<T>(
		Vector4<T>(m_mtx[0].Vector3<T>::operator-(m[0]), m_mtx[0][3]),
		Vector4<T>(m_mtx[1].Vector3<T>::operator-(m[1]), m_mtx[1][3]),
		Vector4<T>(m_mtx[2].Vector3<T>::operator-(m[2]), m_mtx[2][3]),
		m_mtx[3] - m[3]
	);
}

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::operator-(const RigidMatrix4X4<T> &m) const
{
	return operator-(m.m_mtx);
}

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::operator-(const SolidMatrix4X4<T> &m) const
{
	return operator-(m.m_mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::operator*()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::operator*(const Matrix4X4<T> &m) const
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
inline Matrix4X4<T> Matrix4X4<T>::operator*(const AffineMatrix4X4<T> &m) const
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
inline Matrix4X4<T> Matrix4X4<T>::operator*(const RigidMatrix4X4<T> &m) const
{
	return operator*(m.m_mtx);
}

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::operator*(const SolidMatrix4X4<T> &m) const
{
	return operator*(m.m_mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::operator=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator=(const Rows4X4<T> &rows)
{
	m_mtx = rows;
	return *this;
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator=(const Columns4X4<T> &columns)
{
	m_mtx = columns;
	return *this;
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator=(const Matrix4X4<T> &m)
{
	return operator=(m.m_mtx);
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator=(const AffineMatrix4X4<T> &m)
{
	return operator=(m.m_mtx);
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator=(const RigidMatrix4X4<T> &m)
{
	return operator=(m.m_mtx);
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator=(const SolidMatrix4X4<T> &m)
{
	return operator=(m.m_mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::operator+=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator+=(const Matrix4X4<T> &m)
{
	m_mtx[0] += m.m_mtx[0];
	m_mtx[1] += m.m_mtx[1];
	m_mtx[2] += m.m_mtx[2];
	m_mtx[3] += m.m_mtx[3];
	return *this;
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator+=(const AffineMatrix4X4<T> &m)
{
	m_mtx[0].Vector3<T>::operator+=(m[0]);
	m_mtx[1].Vector3<T>::operator+=(m[1]);
	m_mtx[2].Vector3<T>::operator+=(m[2]);
	m_mtx[3] += m[3];
	return *this;
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator+=(const RigidMatrix4X4<T> &m)
{
	return operator+=(m.m_mtx);
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator+=(const SolidMatrix4X4<T> &m)
{
	return operator+=(m.m_mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::operator-=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator-=(const Matrix4X4<T> &m)
{
	m_mtx[0] -= m.m_mtx[0];
	m_mtx[1] -= m.m_mtx[1];
	m_mtx[2] -= m.m_mtx[2];
	m_mtx[3] -= m.m_mtx[3];
	return *this;
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator-=(const AffineMatrix4X4<T> &m)
{

	m_mtx[0].Vector3<T>::operator-=(m[0]);
	m_mtx[1].Vector3<T>::operator-=(m[1]);
	m_mtx[2].Vector3<T>::operator-=(m[2]);
	m_mtx[3] -= m[3];
	return *this;
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator-=(const RigidMatrix4X4<T> &m)
{
	return operator-=(m.m_mtx);
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator-=(const SolidMatrix4X4<T> &m)
{
	return operator-=(m.m_mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::operator*=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator*=(const Matrix4X4<T> &m)
{
	return operator=(*this * m);
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator*=(const AffineMatrix4X4<T> &m)
{
	return operator=(*this * m);
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator*=(const RigidMatrix4X4<T> &m)
{
	return operator=(*this * m);
}

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator*=(const SolidMatrix4X4<T> &m)
{
	return operator=(*this * m);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::operator*()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::operator*(T s) const
{
	return Matrix4X4<T>(
		m_mtx[0] * s,
		m_mtx[1] * s,
		m_mtx[2] * s,
		m_mtx[3] * s
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::operator/()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> Matrix4X4<T>::operator/(T s) const
{
	return operator*(Reciprocal(s));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::operator*=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator*=(T s)
{
	return operator=(*this * s);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::operator/=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Matrix4X4<T> &Matrix4X4<T>::operator/=(T s)
{
	return operator=(*this / s);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::operator*()
//////////////////////////////////////////////////////////////////////////////////////////

// vec * matrix: vec is a row vector, multiplied by each column of matrix.

template <typename T>
inline Vector3<T> Matrix4X4<T>::operator*(const Vector3<T> &v) const
{
	T w = m_mtx[0][3] * v[0] + m_mtx[1][3] * v[1] + m_mtx[2][3] * v[2] + m_mtx[3][3];
	T oow = Reciprocal(w);
  return Vector3<T>(
		(m_mtx[0][0] * v[0] + m_mtx[1][0] * v[1] + m_mtx[2][0] * v[2] + m_mtx[3][0]) * oow,
		(m_mtx[0][1] * v[0] + m_mtx[1][1] * v[1] + m_mtx[2][1] * v[2] + m_mtx[3][1]) * oow,
		(m_mtx[0][2] * v[0] + m_mtx[1][2] * v[1] + m_mtx[2][2] * v[2] + m_mtx[3][2]) * oow
	);
}

// vec * matrix: vec is a row vector, multiplied by each column of matrix.

template <typename T>
inline Vector4<T> Matrix4X4<T>::operator*(const Vector4<T> &v) const
{
  return Vector4<T>(
		(m_mtx[0][0] * v[0] + m_mtx[1][0] * v[1] + m_mtx[2][0] * v[2] + m_mtx[3][0] * v[3]),
		(m_mtx[0][1] * v[0] + m_mtx[1][1] * v[1] + m_mtx[2][1] * v[2] + m_mtx[3][1] * v[3]),
		(m_mtx[0][2] * v[0] + m_mtx[1][2] * v[1] + m_mtx[2][2] * v[2] + m_mtx[3][2] * v[3]),
		(m_mtx[0][3] * v[0] + m_mtx[1][3] * v[1] + m_mtx[2][3] * v[2] + m_mtx[3][3] * v[3])
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>::LookAt()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Matrix4X4<T> Matrix4X4<T>::LookAt(const Vector3<T> &eye, const Vector3<T> &look, const Vector3<T> &up)
{
	Vector3<T> zAxis(look.Unit());
	Vector3<T> xAxis(up.Cross(zAxis));
	xAxis.Normalize();
	Vector3<T> yAxis(zAxis.Cross(xAxis));

	return Matrix4X4<T>(
		Vector3<T>(xAxis.X(), yAxis.X(), zAxis.X()),
		Vector3<T>(xAxis.Y(), yAxis.Y(), zAxis.Y()),
		Vector3<T>(xAxis.Z(), yAxis.Z(), zAxis.Z()),
		Vector3<T>(-xAxis.Dot(eye), -yAxis.Dot(eye), -zAxis.Dot(eye))
	);
}
