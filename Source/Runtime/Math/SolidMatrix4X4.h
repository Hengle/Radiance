// SolidMatrix4X4.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// Sanity check
//////////////////////////////////////////////////////////////////////////////////////////

#if !defined(__RADMATH_MATRIX_H__)
	#error Do not include SolidMatrix4X4.h directly, include through Matrix.h instead
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// math free functions
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Matrix4X4<T> operator*(T s, const SolidMatrix4X4<T> &m);

template <typename T>
Vector3<T> operator*(const Vector3<T> &v, const SolidMatrix4X4<T> &m);

template <typename T>
Vector4<T> operator*(const Vector4<T> &v, const SolidMatrix4X4<T> &m);

//////////////////////////////////////////////////////////////////////////////////////////
// math::SolidMatrix4X4<T>
//////////////////////////////////////////////////////////////////////////////////////////
//
// Solid matrices preserve lines, angles, and lengths.
// Rotation and Translation allowed.
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class SolidMatrix4X4
{

public:

	// Types

	typedef T             ValueType;
	typedef Axis3         AxisType;
	typedef Rows4X4<T>    RowsType;
	typedef Columns4X4<T> ColumnsType;

	// Constructors

	SolidMatrix4X4();
	SolidMatrix4X4(const SolidMatrix4X4<T> &m);

	// Destructor

	~SolidMatrix4X4();

	// Transform matrix creation

	static SolidMatrix4X4<T> Rotation(Axis3 axis, T radians);
	static SolidMatrix4X4<T> Rotation(const AxisAngle<T> &axisAngle);
	static SolidMatrix4X4<T> Rotation(const Quaternion<T> &rot);
	static SolidMatrix4X4<T> Rotation(const Euler<T> &euler);
	static SolidMatrix4X4<T> LookAtLH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up);
    static SolidMatrix4X4<T> LookAtRH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up);
	static SolidMatrix4X4<T> Translation(const Vector3<T> &offset);

	// Access

	const Vector4<T> &operator[](int i) const;
	const Vector4<T> &Row(int i) const;
	Vector4<T> Column(int i) const;
	const Rows4X4<T> &Rows() const;
	Columns4X4<T> Columns() const;
	Quaternion<T> Rotation() const;
	Vector3<T> FrameUp() const;
	Vector3<T> FrameRight() const;
	Vector3<T> FrameForward() const;
	SolidMatrix4X4<T> Inverse() const;
	T Determinant() const;
	T Determinant3X3() const;
	const Matrix4X4<T> &Matrix() const;
	const AffineMatrix4X4<T> &Affine() const;
	const RigidMatrix4X4<T> &Rigid() const;

	// Operations

	SolidMatrix4X4<T> &MakeIdentity();
	SolidMatrix4X4<T> &Invert();

	// Make transform matrix

	SolidMatrix4X4<T> &MakeRotation(Axis3 axis, T radians);
	SolidMatrix4X4<T> &MakeRotation(const AxisAngle<T> &axisAngle);
	SolidMatrix4X4<T> &MakeRotation(const Quaternion<T> &rot);
	SolidMatrix4X4<T> &MakeRotation(const Euler<T> &rot);
	SolidMatrix4X4<T> &MakeLookAtLH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up);
    SolidMatrix4X4<T> &MakeLookAtRH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up);
	SolidMatrix4X4<T> &MakeTranslation(const Vector3<T> &offset);

	// Post-concatenate transformation
	// These are more efficient than the general case matrix post-concatenation.

	SolidMatrix4X4<T> &RotateBy(Axis3 axis, T radians);
	SolidMatrix4X4<T> &TranslateBy(const Vector3<T> &offset);

	// Transform vectors
	// These are more efficient than the general case vector transformation.

	Vector3<T> Rotate(const Vector3<T> &v) const;
	Vector4<T> Rotate(const Vector4<T> &v) const;
	Vector3<T> Translate(const Vector3<T> &v) const;
	Vector4<T> Translate(const Vector4<T> &v) const;
	Vector3<T> Transform(const Vector3<T> &v) const;
	Vector4<T> Transform(const Vector4<T> &v) const;

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
	AffineMatrix4X4<T> operator*(const AffineMatrix4X4<T> &m) const;
	RigidMatrix4X4<T> operator*(const RigidMatrix4X4<T> &m) const;
	SolidMatrix4X4<T> operator*(const SolidMatrix4X4<T> &m) const;

	// Assignment operations

	SolidMatrix4X4<T> &operator=(const SolidMatrix4X4<T> &m);

	SolidMatrix4X4<T> &operator*=(const SolidMatrix4X4<T> &m);

	// Scalar binary operations

	Matrix4X4<T> operator*(T s) const;
	Matrix4X4<T> operator/(T s) const;
	template <typename X>
	friend Matrix4X4<X> operator*(X s, const SolidMatrix4X4<X> &m);

	// Vector binary operations

	Vector3<T> operator*(const Vector3<T> &v) const;
	Vector4<T> operator*(const Vector4<T> &v) const;

private:

	explicit SolidMatrix4X4(const RigidMatrix4X4<T> &mtx);
	explicit SolidMatrix4X4(const Rows4X4<T> &rows);
	explicit SolidMatrix4X4(const Columns4X4<T> &columns);

	SolidMatrix4X4<T> &Initialize(
		const Vector3<T> &row0,
		const Vector3<T> &row1,
		const Vector3<T> &row2,
		const Vector3<T> &row3
	);

	RigidMatrix4X4<T> m_mtx;
	friend class Matrix4X4<T>;
	friend class AffineMatrix4X4<T>;
	friend class RigidMatrix4X4<T>;
};
