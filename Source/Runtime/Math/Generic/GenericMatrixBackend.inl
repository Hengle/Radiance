// GenericMatrixBackend.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// math::GenericMatrix4X4Traits<M, T>::MakeTransformation()
//////////////////////////////////////////////////////////////////////////////////////////

template <template <typename> class M, typename T>
M<T> &GenericMatrix4X4Traits<M, T>::MakeTransformation(
	M<T>                &matrix,
	T                   scaling,
	const Vector3<T>    *rotationCenter,
	const Quaternion<T> *rotation,
	const Vector3<T>    *translation
)
{
	matrix.MakeIdentity();
	if (scaling != T(1.0))
	{
		matrix.ScaleBy(Scale3<T>(scaling));
	}
	if (rotation)
	{
		if (rotationCenter)
		{
			matrix.TranslateBy(-*rotationCenter);
		}
		matrix *= M<T>::Rotation(*rotation);
		if (rotationCenter)
		{
			matrix.TranslateBy(*rotationCenter);
		}
	}
	if (translation)
	{
		matrix.TranslateBy(*translation);
	}
	return matrix;
}

template <template <typename> class M, typename T>
M<T> &GenericMatrix4X4Traits<M, T>::MakeTransformation(
	M<T>                &matrix,
	const Vector3<T>    *scalingCenter,
	const Quaternion<T> *scalingRotation,
	const Vector3<T>    *scaling,
	const Vector3<T>    *rotationCenter,
	const Quaternion<T> *rotation,
	const Vector3<T>    *translation
)
{
	matrix.MakeIdentity();
	if (scaling)
	{
		if (scalingCenter)
		{
			matrix.TranslateBy(-*scalingCenter);
		}
		M<T> Msr;
		if (scalingRotation)
		{
			Msr.MakeRotation(*scalingRotation);
			matrix *= M<T>::Rotation(Msr.Inverse());
		}
		matrix.ScaleBy(*scaling);
		if (scalingRotation)
		{
			matrix *= M<T>::Rotation(Msr);
		}
		if (scalingCenter)
		{
			matrix.TranslateBy(*scalingCenter);
		}
	}
	if (rotation)
	{
		if (rotationCenter)
		{
			matrix.TranslateBy(-*rotationCenter);
		}
		matrix *= M<T>::Rotation(*rotation);
		if (rotationCenter)
		{
			matrix.TranslateBy(*rotationCenter);
		}
	}
	if (translation)
	{
		matrix.TranslateBy(*translation);
	}
	return matrix;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::GenericMatrix4X4Traits<M, T>::MakeTransformation2D()
//////////////////////////////////////////////////////////////////////////////////////////

template <template <typename> class M, typename T>
M<T> &GenericMatrix4X4Traits<M, T>::MakeTransformation2D(
	M<T>             &matrix,
	T                scaling,
	const Vector2<T> *rotationCenter,
	T                rotation,
	const Vector2<T> *translation
)
{
	matrix.MakeIdentity();
	if (scaling != T(1.0))
	{
		matrix.ScaleBy(Scale3<T>(scaling, scaling, T(1.0)));
	}
	if (rotationCenter)
	{
		matrix.TranslateBy(Vector3<T>(-*rotationCenter, T(0.0)));
	}
	if (rotation)
	{
		matrix.RotateBy(AxisZ, rotation);
	}
	if (rotationCenter)
	{
		matrix.TranslateBy(Vector3<T>(*rotationCenter, T(0.0)));
	}
	if (translation)
	{
		matrix.TranslateBy(Vector3<T>(*translation, T(0.0)));
	}
	return matrix;
}

template <template <typename> class M, typename T>
M<T> &GenericMatrix4X4Traits<M, T>::MakeTransformation2D(
	M<T>             &matrix,
	const Vector2<T> *scalingCenter,
	T                scalingRotation,
	const Vector2<T> *scaling,
	const Vector2<T> *rotationCenter,
	T                rotation,
	const Vector2<T> *translation
)
{
	matrix.MakeIdentity();
	if (scaling)
	{
		if (scalingCenter)
		{
			matrix.TranslateBy(Vector3<T>(-*scalingCenter, T(0.0)));
		}
		if (scalingRotation != T(0.0))
		{
			matrix.RotateBy(AxisZ, -scalingRotation);
		}
		matrix.ScaleBy(Vector3<T>(*scaling, 1.0f));
		if (scalingRotation != T(0.0))
		{
			matrix.RotateBy(AxisZ, scalingRotation);
		}
		if (scalingCenter)
		{
			matrix.TranslateBy(Vector3<T>(*scalingCenter, T(0.0)));
		}
	}
	if (rotation != T(0.0))
	{
		if (rotationCenter)
		{
			matrix.TranslateBy(Vector3<T>(-*rotationCenter, T(0.0)));
		}
		matrix.RotateBy(AxisZ, rotation);
		if (rotationCenter)
		{
			matrix.TranslateBy(Vector3<T>(*rotationCenter, T(0.0)));
		}
	}
	if (translation)
	{
		matrix.TranslateBy(Vector3<T>(*translation, T(0.0)));
	}
	return matrix;
}
