// AffineMatrix4X4.h
// 4X4 Affine Matrix classes
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// Sanity check
//////////////////////////////////////////////////////////////////////////////////////////

#if !defined(__RADMATH_MATRIX_H__)
	#error Do not include AffineMatrix4X4.h directly, include through Matrix.h instead
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// math free functions
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Matrix4X4<T> operator*(T s, const AffineMatrix4X4<T> &m);

template <typename T>
Vector3<T> operator*(const Vector3<T> &v, const AffineMatrix4X4<T> &m);

template <typename T>
Vector4<T> operator*(const Vector4<T> &v, const AffineMatrix4X4<T> &m);

// Affine matrices preserve lines.
// Rotation, Nonuniform Scale (s != 0), and Translation allowed.

template <typename T>
class AffineMatrix4X4
{

public:

	// Traits

	typedef typename MatrixBackendSelector<T>::AffineMatrix4X4TraitsType TraitsType;

	// Types

	typedef T             ValueType;
	typedef Axis3         AxisType;
	typedef Rows4X4<T>    RowsType;
	typedef Columns4X4<T> ColumnsType;

	// Constructors

	AffineMatrix4X4();
	AffineMatrix4X4(const AffineMatrix4X4<T> &m);
	explicit AffineMatrix4X4(const RigidMatrix4X4<T> &m);
	explicit AffineMatrix4X4(const SolidMatrix4X4<T> &m);

	// Destructor

	~AffineMatrix4X4();

	// Projection matrix creation

	static AffineMatrix4X4<T> OrthoLH(T width, T height, T zNear, T zFar);
	static AffineMatrix4X4<T> OrthoRH(T width, T height, T zNear, T zFar);
	static AffineMatrix4X4<T> OrthoOffCenterLH(T left, T right, T bottom, T top, T zNear, T zFar);
	static AffineMatrix4X4<T> OrthoOffCenterRH(T left, T right, T bottom, T top, T zNear, T zFar);

	// Transform matrix creation

	static AffineMatrix4X4<T> Rotation(Axis3 axis, T radians);
	static AffineMatrix4X4<T> Rotation(const AxisAngle<T> &axisAngle);
	static AffineMatrix4X4<T> Rotation(const Quaternion<T> &rot);
	static AffineMatrix4X4<T> Rotation(const Euler<T> &euler);
	static AffineMatrix4X4<T> LookAtLH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up);
    static AffineMatrix4X4<T> LookAtRH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up);
	static AffineMatrix4X4<T> Scaling(const Scale3<T> &scale);
	static AffineMatrix4X4<T> Translation(const Vector3<T> &offset);

	static AffineMatrix4X4<T> Transformation(
		T                   scaling,
		const Vector3<T>    *rotationCenter,
		const Quaternion<T> *rotation,
		const Vector3<T>    *translation
	);

	static AffineMatrix4X4<T> Transformation(
		const Vector3<T>    *scalingCenter,
		const Quaternion<T> *scalingRotation,
		const Vector3<T>    *scaling,
		const Vector3<T>    *rotationCenter,
		const Quaternion<T> *rotation,
		const Vector3<T>    *translation
	);

	static AffineMatrix4X4<T> Transformation2D(
		T                scaling,
		const Vector2<T> *rotationCenter,
		T                rotation,
		const Vector2<T> *translation
	);

	static AffineMatrix4X4<T> Transformation2D(
		const Vector2<T> *scalingCenter,
		T                scalingRotation,
		const Vector2<T> *scaling,
		const Vector2<T> *rotationCenter,
		T                rotation,
		const Vector2<T> *translation
	);

	// Access

	const Vector4<T> &operator[](int i) const;
	const Vector4<T> &Row(int i) const;
	Vector4<T> Column(int i) const;
	const Rows4X4<T> &Rows() const;
	Columns4X4<T> Columns() const;
	Quaternion<T> Rotation() const;
	AffineMatrix4X4<T> Inverse() const;
	T Determinant() const;
	T Determinant3X3() const;
	const Matrix4X4<T> &Matrix() const;

	// Operations

	AffineMatrix4X4<T> &MakeIdentity();
	AffineMatrix4X4<T> &Invert();

	// Make projection matrix

	AffineMatrix4X4<T> &MakeOrthoLH(T width, T height, T zNear, T zFar);
	AffineMatrix4X4<T> &MakeOrthoRH(T width, T height, T zNear, T zFar);
	AffineMatrix4X4<T> &MakeOrthoOffCenterLH(T left, T right, T bottom, T top, T zNear, T zFar);
	AffineMatrix4X4<T> &MakeOrthoOffCenterRH(T left, T right, T bottom, T top, T zNear, T zFar);

	// Make transform matrix

	AffineMatrix4X4<T> &MakeRotation(Axis3 axis, T radians);
	AffineMatrix4X4<T> &MakeRotation(const AxisAngle<T> &axisAngle);
	AffineMatrix4X4<T> &MakeRotation(const Quaternion<T> &rot);
	AffineMatrix4X4<T> &MakeRotation(const Euler<T> &rot);
	AffineMatrix4X4<T> &MakeLookAtLH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up);
    AffineMatrix4X4<T> &MakeLookAtRH(const Vector3<T> &eye, const Vector3<T> &at, const Vector3<T> &up);
	AffineMatrix4X4<T> &MakeScaling(const Scale3<T> &scale);
	AffineMatrix4X4<T> &MakeTranslation(const Vector3<T> &offset);

	AffineMatrix4X4<T> &MakeTransformation(
		T                   scaling,
		const Vector3<T>    *rotationCenter,
		const Quaternion<T> *rotation,
		const Vector3<T>    *translation
	);

	AffineMatrix4X4<T> &MakeTransformation(
		const Vector3<T>    *scalingCenter,
		const Quaternion<T> *scalingRotation,
		const Vector3<T>    *scaling,
		const Vector3<T>    *rotationCenter,
		const Quaternion<T> *rotation,
		const Vector3<T>    *translation
	);

	AffineMatrix4X4<T> &MakeTransformation2D(
		T                scaling,
		const Vector2<T> *rotationCenter,
		T                rotation,
		const Vector2<T> *translation
	);

	AffineMatrix4X4<T> &MakeTransformation2D(
		const Vector2<T> *scalingCenter,
		T                scalingRotation,
		const Vector2<T> *scaling,
		const Vector2<T> *rotationCenter,
		T                rotation,
		const Vector2<T> *translation
	);

	// Post-concatenate transformation
	// These are more efficient than the general case matrix post-concatenation.

	AffineMatrix4X4<T> &RotateBy(Axis3 axis, T radians);
	AffineMatrix4X4<T> &ScaleBy(const Vector3<T> &scale);
	AffineMatrix4X4<T> &TranslateBy(const Vector3<T> &offset);

	// Transform vectors
	// These are more efficient than the general case vector transformation.

	Vector3<T> ScaleAndRotate(const Vector3<T> &v) const;
	Vector4<T> ScaleAndRotate(const Vector4<T> &v) const;
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
	AffineMatrix4X4<T> operator*(const RigidMatrix4X4<T> &m) const;
	AffineMatrix4X4<T> operator*(const SolidMatrix4X4<T> &m) const;

	// Assignment operations

	AffineMatrix4X4<T> &operator=(const AffineMatrix4X4<T> &m);
	AffineMatrix4X4<T> &operator=(const RigidMatrix4X4<T> &m);
	AffineMatrix4X4<T> &operator=(const SolidMatrix4X4<T> &m);

	AffineMatrix4X4<T> &operator*=(const AffineMatrix4X4<T> &m);
	AffineMatrix4X4<T> &operator*=(const RigidMatrix4X4<T> &m);
	AffineMatrix4X4<T> &operator*=(const SolidMatrix4X4<T> &m);

	// Scalar binary operations

	Matrix4X4<T> operator*(T s) const;
	Matrix4X4<T> operator/(T s) const;

	// Vector binary operations

	Vector3<T> operator*(const Vector3<T> &v) const;
	Vector4<T> operator*(const Vector4<T> &v) const;

private:

	explicit AffineMatrix4X4(const Matrix4X4<T> &mtx);
	explicit AffineMatrix4X4(const Rows4X4<T> &rows);
	explicit AffineMatrix4X4(const Columns4X4<T> &columns);

	AffineMatrix4X4<T> &Initialize(
		const Vector3<T> &row0,
		const Vector3<T> &row1,
		const Vector3<T> &row2,
		const Vector3<T> &row3
	);

#if defined(RAD_OPT_DEBUG)
	bool IsValid() const;
#endif

	Matrix4X4<T> m_mtx;
	friend class Matrix4X4<T>;
	friend class RigidMatrix4X4<T>;
	friend class SolidMatrix4X4<T>;
};
