// Euler.inl
// Inlines for Euler.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math::Euler<T>::Euler()
//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline Euler<T>::Euler() :
Vector3<T>()
{
}

template<typename T>
inline Euler<T>::Euler(const T *v) :
Vector3<T>(v)
{	
}

template<typename T>
inline Euler<T>::Euler(T pitch, T yaw, T roll) :
Vector3<T>(roll, pitch, yaw)
{
}

template<typename T>
inline Euler<T>::Euler(const Euler<T> &e) :
Vector3<T>(e)
{
}

template<typename T>
inline Euler<T>::Euler(const Vector3<T> &v) :
Vector3<T>(v)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Euler<T>::~Euler()
//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline Euler<T>::~Euler()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Euler<T>::Pitch()
//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline T Euler<T>::Pitch() const
{
    return this->Y();
}

template<typename T>
inline void Euler<T>::Pitch(T pitch)
{
    this->SetY(pitch);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Euler<T>::Yaw()
//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline T Euler<T>::Yaw() const
{
    return this->Z();
}

template<typename T>
inline void Euler<T>::Yaw(T yaw)
{
    this->SetZ(yaw);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Euler<T>::Roll()
//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline T Euler<T>::Roll() const
{
    return this->X();
}

template<typename T>
inline void Euler<T>::Roll(T roll)
{
    this->SetX(roll);
}

} // math

