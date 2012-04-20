// Matrix4X4.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// Sanity check
//////////////////////////////////////////////////////////////////////////////////////////

#if !defined(__RADMATH_MATRIX_H__)
	#error Do not include Matrix4X4.h directly, include through Matrix.h instead
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// math free functions
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Matrix4X4<T> operator*(T s, const Matrix4X4<T> &m);

template <typename T>
Vector3<T> operator*(const Vector3<T> &v, const Matrix4X4<T> &m);

template <typename T>
Vector4<T> operator*(const Vector4<T> &v, const Matrix4X4<T> &m);

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4<T>
//////////////////////////////////////////////////////////////////////////////////////////
//
// No guarantees or constraints, just a general purpose 4x4 matrix.
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T> class Matrix4X4;

template <typename T>
class Matrix4X4
{

public:

	// Traits

	typedef typename MatrixBackendSelector<T>::Matrix4X4TraitsType TraitsType;

	// Types

	typedef T             ValueType;
	typedef Axis3         AxisType;
	typedef Rows4X4<T>    RowsType;
	typedef Columns4X4<T> ColumnsType;

	// Constants

	static const Matrix4X4<T> Identity;
	static const Matrix4X4<T> Zero;

	// Constructors

	Matrix4X4();
	Matrix4X4(const Matrix4X4<T> &m);
	explicit Matrix4X4(const AffineMatrix4X4<T> &m);
	explicit Matrix4X4(const RigidMatrix4X4<T> &m);
	explicit Matrix4X4(const SolidMatrix4X4<T> &m);
	explicit Matrix4X4(const Rows4X4<T> &rows);
	explicit Matrix4X4(const Columns4X4<T> &columns);
	Matrix4X4(
		const Vector4<T> &row0,
		const Vector4<T> &row1,
		const Vector4<T> &row2,
		const Vector4<T> &row3
	);
	Matrix4X4(
		const Vector3<T> &row0,
		const Vector3<T> &row1,
		const Vector3<T> &row2,
		const Vector3<T> &row3
	);

	// Destructor

	~Matrix4X4();

	// Projection matrix creation

	static Matrix4X4<T> Ortho(T left, T right, T bottom, T top, T zNear, T zFar);
	static Matrix4X4<T> PerspectiveFovLH(T fovY, T aspect, T zNear, T zFar);
	static Matrix4X4<T> PerspectiveFovRH(T fovY, T aspect, T zNear, T zFar);
	static Matrix4X4<T> PerspectiveLH(T width, T height, T zNear, T zFar);
	static Matrix4X4<T> PerspectiveRH(T width, T height, T zNear, T zFar);
	static Matrix4X4<T> PerspectiveOffCenterLH(T left, T right, T bottom, T top, T zNear, T zFar);
	static Matrix4X4<T> PerspectiveOffCenterRH(T left, T right, T bottom, T top, T zNear, T zFar);

	// Transform matrix creation

	static Matrix4X4<T> Rotation(Axis3 axis, T radians);
	static Matrix4X4<T> Rotation(const AxisAngle<T> &axisAngle);
	static Matrix4X4<T> Rotation(const Quaternion<T> &quaternion);
	static Matrix4X4<T> Rotation(const Euler<T> &euler);
    static Matrix4X4<T> LookAtLH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up);
    static Matrix4X4<T> LookAtRH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up);
	static Matrix4X4<T> Scaling(const Scale3<T> &scale);
	static Matrix4X4<T> Translation(const Vector3<T> &offset);

	static Matrix4X4<T> Transformation(
		T                   scaling,
		const Vector3<T>    *rotationCenter,
		const Quaternion<T> *rotation,
		const Vector3<T>    *translation
	);

	static Matrix4X4<T> Transformation(
		const Vector3<T>    *scalingCenter,
		const Quaternion<T> *scalingRotation,
		const Vector3<T>    *scaling,
		const Vector3<T>    *rotationCenter,
		const Quaternion<T> *rotation,
		const Vector3<T>    *translation
	);

	static Matrix4X4<T> Transformation2D(
		T                scaling,
		const Vector2<T> *rotationCenter,
		T                rotation,
		const Vector2<T> *translation
	);

	static Matrix4X4<T> Transformation2D(
		const Vector2<T> *scalingCenter,
		T                scalingRotation,
		const Vector2<T> *scaling,
		const Vector2<T> *rotationCenter,
		T                rotation,
		const Vector2<T> *translation
	);

	// Access

	const Vector4<T> &operator[](int i) const;
	Vector4<T> &operator[](int i);
	const Vector4<T> &Row(int i) const;
	Vector4<T> Column(int i) const;
	const Rows4X4<T> &Rows() const;
	Columns4X4<T> Columns() const;
	Quaternion<T> Rotation() const;
	Vector3<T> Angles() const;
	Matrix4X4<T> Transpose() const;
	Matrix4X4<T> Inverse() const;
	T Determinant() const;
	T Determinant3X3() const;

	// Operations

	Matrix4X4<T> &Initialize(
		const Vector4<T> &row0,
		const Vector4<T> &row1,
		const Vector4<T> &row2,
		const Vector4<T> &row3
	);
	Matrix4X4<T> &Initialize(
		const Vector3<T> &row0,
		const Vector3<T> &row1,
		const Vector3<T> &row2,
		const Vector3<T> &row3
	);
	Matrix4X4<T> &MakeIdentity();
	Matrix4X4<T> &MakeTranspose();
	Matrix4X4<T> &Invert();

	// Make projection matrix

	Matrix4X4<T> &MakeOrthoLH(T width, T height, T zNear, T zFar);
	Matrix4X4<T> &MakeOrthoRH(T width, T height, T zNear, T zFar);
	Matrix4X4<T> &MakeOrthoOffCenterLH(T left, T right, T bottom, T top, T zNear, T zFar);
	Matrix4X4<T> &MakeOrthoOffCenterRH(T left, T right, T bottom, T top, T zNear, T zFar);
	Matrix4X4<T> &MakePerspectiveFovLH(T fovY, T aspect, T zNear, T zFar);
	Matrix4X4<T> &MakePerspectiveFovRH(T fovY, T aspect, T zNear, T zFar);
	Matrix4X4<T> &MakePerspectiveLH(T width, T height, T zNear, T zFar);
	Matrix4X4<T> &MakePerspectiveRH(T width, T height, T zNear, T zFar);
	Matrix4X4<T> &MakePerspectiveOffCenterLH(T left, T right, T bottom, T top, T zNear, T zFar);
	Matrix4X4<T> &MakePerspectiveOffCenterRH(T left, T right, T bottom, T top, T zNear, T zFar);

	// Make transform matrix

	Matrix4X4<T> &MakeRotation(Axis3 axis, T radians);
	Matrix4X4<T> &MakeRotation(const AxisAngle<T> &axisAngle);
	Matrix4X4<T> &MakeRotation(const Quaternion<T> &rot);
	Matrix4X4<T> &MakeRotation(const Euler<T> &rot);
	Matrix4X4<T> &MakeLookAtLH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up);
    Matrix4X4<T> &MakeLookAtRH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up);
	Matrix4X4<T> &MakeScaling(const Scale3<T> &scale);
	Matrix4X4<T> &MakeTranslation(const Vector3<T> &offset);

	Matrix4X4<T> &MakeTransformation(
		T                   scaling,
		const Vector3<T>    *rotationCenter,
		const Quaternion<T> *rotation,
		const Vector3<T>    *translation
	);

	Matrix4X4<T> &MakeTransformation(
		const Vector3<T>    *scalingCenter,
		const Quaternion<T> *scalingRotation,
		const Vector3<T>    *scaling,
		const Vector3<T>    *rotationCenter,
		const Quaternion<T> *rotation,
		const Vector3<T>    *translation
	);

	Matrix4X4<T> &MakeTransformation2D(
		T                scaling,
		const Vector2<T> *rotationCenter,
		T                rotation,
		const Vector2<T> *translation
	);

	Matrix4X4<T> &MakeTransformation2D(
		const Vector2<T> *scalingCenter,
		T                scalingRotation,
		const Vector2<T> *scaling,
		const Vector2<T> *rotationCenter,
		T                rotation,
		const Vector2<T> *translation
	);

	// Post-concatenate transformation
	// These are more efficient than the general case matrix post-concatenation.

	Matrix4X4<T> &RotateBy(Axis3 axis, T radians);
	Matrix4X4<T> &ScaleBy(const Vector3<T> &scale);
	Matrix4X4<T> &TranslateBy(const Vector3<T> &offset);

	// Transform vectors

	Vector3<T> Transform(const Vector3<T> &v) const;
	Vector4<T> Transform(const Vector4<T> &v) const;
	Vector3<T> Transform3X3(const Vector3<T> &v) const;
	Vector4<T> Transform3X3(const Vector4<T> &v) const;
	Vector3<T> Transform4X3(const Vector3<T> &v) const;
	Vector4<T> Transform4X3(const Vector4<T> &v) const;

	// Binary operations

	Matrix4X4<T> operator+(const Matrix4X4<T> &m) const;
	Matrix4X4<T> operator+(const AffineMatrix4X4<T> &m) const;
	Matrix4X4<T> operator+(const RigidMatrix4X4<T> &m) const;
	Matrix4X4<T> operator+(const SolidMatrix4X4<T> &m) const;

	Matrix4X4<T> operator-(const Matrix4X4<T> &m) const;
	Matrix4X4<T> operator-(const AffineMatrix4X4<T> &m) const;
	Matrix4X4<T> operator-(const RigidMatrix4X4<T> &m) const;
	Matrix4X4<T> operator-(const SolidMatrix4X4<T> &m) const;

	Matrix4X4<T> operator*(const Matrix4X4<T> &m) const;
	Matrix4X4<T> operator*(const AffineMatrix4X4<T> &m) const;
	Matrix4X4<T> operator*(const RigidMatrix4X4<T> &m) const;
	Matrix4X4<T> operator*(const SolidMatrix4X4<T> &m) const;

	// Assignment operations

	Matrix4X4<T> &operator=(const Rows4X4<T> &rows);
	Matrix4X4<T> &operator=(const Columns4X4<T> &columns);

	Matrix4X4<T> &operator=(const Matrix4X4<T> &m);
	Matrix4X4<T> &operator=(const AffineMatrix4X4<T> &m);
	Matrix4X4<T> &operator=(const RigidMatrix4X4<T> &m);
	Matrix4X4<T> &operator=(const SolidMatrix4X4<T> &m);

	Matrix4X4<T> &operator+=(const Matrix4X4<T> &m);
	Matrix4X4<T> &operator+=(const AffineMatrix4X4<T> &m);
	Matrix4X4<T> &operator+=(const RigidMatrix4X4<T> &m);
	Matrix4X4<T> &operator+=(const SolidMatrix4X4<T> &m);

	Matrix4X4<T> &operator-=(const Matrix4X4<T> &m);
	Matrix4X4<T> &operator-=(const AffineMatrix4X4<T> &m);
	Matrix4X4<T> &operator-=(const RigidMatrix4X4<T> &m);
	Matrix4X4<T> &operator-=(const SolidMatrix4X4<T> &m);

	Matrix4X4<T> &operator*=(const Matrix4X4<T> &m);
	Matrix4X4<T> &operator*=(const AffineMatrix4X4<T> &m);
	Matrix4X4<T> &operator*=(const RigidMatrix4X4<T> &m);
	Matrix4X4<T> &operator*=(const SolidMatrix4X4<T> &m);

	// Scalar binary operations

	Matrix4X4<T> operator*(T s) const;
	Matrix4X4<T> operator/(T s) const;

	// Scalar assignment operations

	Matrix4X4<T> &operator*=(T s);
	Matrix4X4<T> &operator/=(T s);

	// Vector binary operations

	Vector3<T> operator*(const Vector3<T> &v) const;
	Vector4<T> operator*(const Vector4<T> &v) const;

private:

	static Matrix4X4<T> LookAt(const Vector3<T> &eye, const Vector3<T> &look, const Vector3<T> &up);

	Rows4X4<T> m_mtx;
	friend class AffineMatrix4X4<T>;
	friend class RigidMatrix4X4<T>;
	friend class SolidMatrix4X4<T>;
};
