// GenericMatrixBackend.h
// Generic matrix backend
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// Sanity check
//////////////////////////////////////////////////////////////////////////////////////////

#if !defined(__RADMATH_MATRIX_H__)
	#error Do not include GenericMatrixBackend.h directly, include through Matrix.h instead
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// math::Matrix4X4Traits<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <template <typename> class M, typename T>
struct GenericMatrix4X4Traits
{
	static M<T> &MakeTransformation(
		M<T>                &matrix,
		T                   scaling,
		const Vector3<T>    *rotationCenter,
		const Quaternion<T> *rotation,
		const Vector3<T>    *translation
	);

	static M<T> &MakeTransformation(
		M<T>                &matrix,
		const Vector3<T>    *scalingCenter,
		const Quaternion<T> *scalingRotation,
		const Vector3<T>    *scaling,
		const Vector3<T>    *rotationCenter,
		const Quaternion<T> *rotation,
		const Vector3<T>    *translation
	);

	static M<T> &MakeTransformation2D(
		M<T>             &matrix,
		T                scaling,
		const Vector2<T> *rotationCenter,
		T                rotation,
		const Vector2<T> *translation
	);

	static M<T> &MakeTransformation2D(
		M<T>             &matrix,
		const Vector2<T> *scalingCenter,
		T                scalingRotation,
		const Vector2<T> *scaling,
		const Vector2<T> *rotationCenter,
		T                rotation,
		const Vector2<T> *translation
	);
};
