// Math.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#include <math.h>

#include "../TypeTraits.h"
#include "../Base/Utils.h"

#if defined(RAD_OPT_WINX)
	#include <intrin.h>
#endif

#include "../PushSystemMacros.h"


namespace math {

template <typename T>
inline T RadToDeg(const T &rad) {
	return rad / Constants<T>::PI() * T(180.0);
}

template <typename T>
inline T DegToRad(const T &deg) {
	return deg / T(180.0) * Constants<T>::PI();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Epsilon()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T Epsilon() {
	return T(0.0);
}

template <>
inline F32 Epsilon() {
	return std::numeric_limits<F32>::epsilon();
}

template <>
inline F64 Epsilon() {
	return std::numeric_limits<F64>::epsilon();
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Abs()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T Abs(const T &n) {
	return (n < T(0.0) ? -n : n);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::BitScanForward()
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_WINX) && (RAD_OPT_MACHINE_WORD_SIZE == 4)

template <>
inline bool BitScanForward(U32 &bit, const U32 &n) {
	return (0 != ::_BitScanForward(reinterpret_cast<unsigned long *>(&bit), n));
}

template <>
inline bool BitScanForward(U64 &bit, const U64 &n) {
	U32 rbit;
	if (BitScanForward(rbit, static_cast<U32>(n))) {
		bit = static_cast<U64>(rbit);
		return true;
	}

	if (BitScanForward(rbit, static_cast<U32>(n >> 32))) {
		bit = static_cast<U64>(rbit) + 32L;
		return true;
	}

	bit = 0;
	return false;
}

template <>
inline bool BitScanForward(S64 &bit, const S64 &n) {
	return BitScanForward(*reinterpret_cast<U64 *>(&bit), *reinterpret_cast<const U64 *>(&n));
}

template <typename T>
inline bool BitScanForward(T &bit, const T &n) {
	RAD_STATIC_ASSERT(meta::IsInteger<T>::VALUE);
	typedef meta::IntegerTraits<T>::UnsignedType UT;
	U32 rbit;
	const UT &un = *reinterpret_cast<const UT *>(&n);
	if (BitScanForward(rbit, U32(un))) {
		bit = static_cast<T>(rbit);
		return true;
	}
	return false;
}

#else // !defined(RAD_OPT_WINX) || (RAD_OPT_MACHINE_WORD_SIZE != 4)

template <typename T>
bool BitScanForward(T &bit, const T &n) {
	RAD_STATIC_ASSERT(meta::IsInteger<T>::VALUE);
	const int BITS = meta::IntegerTraits<T>::NUM_BITS;
	for (int i = 0; i < BITS; ++i) {
		if (0 != ((n >> i) & T(1))) {
			bit = T(i);
			return true;
		}
	}
	return false;
}

#endif // !defined(RAD_OPT_WINX) && (RAD_OPT_MACHINE_WORD_SIZE == 4)

//////////////////////////////////////////////////////////////////////////////////////////
// math::BitScanReverse()
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_WIN) && (RAD_OPT_MACHINE_WORD_SIZE == 4)

template <>
inline bool BitScanReverse(U32 &bit, const U32 &n) {
	return (0 != ::_BitScanReverse(reinterpret_cast<unsigned long *>(&bit), n));
}

template <>
inline bool BitScanReverse(U64 &bit, const U64 &n) {
	U32 rbit;
	if (BitScanReverse(rbit, static_cast<U32>(n >> 32))) {
		bit = static_cast<U64>(rbit) + 32L;
		return true;
	}
	if (BitScanReverse(rbit, static_cast<U32>(n))) {
		bit = static_cast<U64>(rbit);
		return true;
	}
	bit = 0;
	return false;
}

template <>
inline bool BitScanReverse(S64 &bit, const S64 &n) {
	return BitScanReverse(*reinterpret_cast<U64 *>(&bit), *reinterpret_cast<const U64 *>(&n));
}

template <typename T>
inline bool BitScanReverse(T &bit, const T &n) {
	RAD_STATIC_ASSERT(meta::IsInteger<T>::VALUE);
	typedef meta::IntegerTraits<T>::UnsignedType UT;
	U32 rbit;
	const UT &un = *reinterpret_cast<const UT *>(&n);
	if (BitScanReverse(rbit, U32(un))) {
		bit = static_cast<T>(rbit);
		return true;
	}
	return false;
}

#else // !defined(RAD_OPT_WIN) || (RAD_OPT_MACHINE_WORD_SIZE != 4)

template <typename T>
bool BitScanReverse(T &bit, const T &n)
{
	RAD_STATIC_ASSERT(meta::IsInteger<T>::VALUE);
	const int BITS = meta::IntegerTraits<T>::NUM_BITS;
	for (int i = BITS - 1; i >= 0; ++i)
	{
		if (0 != ((n >> i) & T(1)))
		{
			bit = T(i);
			return true;
		}
	}
	return false;
}

#endif // !defined(RAD_OPT_WIN) && (RAD_OPT_MACHINE_WORD_SIZE == 4)

template <typename T>
inline const T &Clamp(const T &n, const T &min, const T &max) {
	return Min(Max(min, n), max);
}

template <typename T>
inline const T &Min(const T &a, const T &b) {
	return (a < b ? a : b);
}

template <typename T>
inline const T &Max(const T &a, const T &b) {
	return (a > b ? a : b);
}

template <>
inline F32 Floor(const F32 &n) {
	return floorf(n);
}

template <>
inline F64 Floor(const F64 &n) {
	return floor(n);
}

template <typename T>
inline T Round(const T &x) {
	return Floor(x + T(1) / T(2));
}

template <>
inline F32 Ceil(const F32 &n) {
	return ceilf(n);
}

template <>
inline F64 Ceil(const F64 &n) {
	return ceil(n);
}

template <typename T>
inline bool NearlyEquals(const T &a, const T &b, const T &tolerance) {
	return (Abs(a - b) < tolerance);
}

template <typename T>
inline bool NearlyZero(const T &n, const T &tolerance) {
	return (Abs(n) < tolerance);
}

template <typename T1, class T2>
inline T1 Quantize(const T2 &n) {
	return static_cast<T1>(n);
}

template <>
inline int Quantize<int, F32>(const F32 &n) {
	return static_cast<int>(n < 0 ? n - 0.5f : n + 0.5f);
}

template <>
inline int Quantize<int, F64>(const F64 &n) {
	return static_cast<int>(n < 0 ? n - 0.5 : n + 0.5);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Reciprocal()
//////////////////////////////////////////////////////////////////////////////////////////

inline F32 Reciprocal(const F32 &n) {
	return (1.0f / n);
}

inline F64 Reciprocal(const F64 &n) {
	return (1.0 / n);
}

inline F32 Reciprocal(const int &n) {
	return Reciprocal(static_cast<F32>(n));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Mod()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T Mod(const T &x, const T &y) {
	return x % y;
}

template <>
inline F32 Mod(const F32 &x, const F32 &y){
	F32 z = x/y;
	return (z - (float)FloatToInt(z)) * y;
}

template <>
inline F64 Mod(const F64 &x, const F64 &y) {
	F64 z = x/y;
	return (z - (float)((int)z)) * y;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::ModF()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline void ModF(T &wholePart, T &fracPart, const T &n) {
	F64 i, f;
	ModF(i, f, F64(n));
	wholePart = T(i);
	return T(f);
}

template <>
inline void ModF(F32 &wholePart, F32 &fracPart, const F32 &n) {
	fracPart = modff(n, &wholePart);
}

template <>
inline void ModF(F64 &wholePart, F64 &fracPart, const F64 &n) {
	fracPart = modf(n, &wholePart);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::SinAndCos()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline void SinAndCos(T *pSin, T *pCos, const T &angle) {
	RAD_ASSERT(pSin);
	RAD_ASSERT(pCos);
	*pSin = Sin(angle);
	*pCos = Cos(angle);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Sin()
//////////////////////////////////////////////////////////////////////////////////////////

template <>
inline F32 Sin<F32>(const F32 &angle) {
	return sinf(angle);
}

template <>
inline F64 Sin<F64>(const F64 &angle) {
	return sin(angle);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Cos()
//////////////////////////////////////////////////////////////////////////////////////////

template <>
inline F32 Cos<F32>(const F32 &angle) {
	return cosf(angle);
}

template <>
inline F64 Cos<F64>(const F64 &angle) {
	return cos(angle);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Tan()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T Tan(const T &angle) {
	return T(tan(F64(angle)));
}

template <>
inline F32 Tan<F32>(const F32 &angle) {
	return tanf(angle);
}

template <>
inline F64 Tan<F64>(const F64 &angle) {
	return tan(angle);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::ArcTan()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T ArcTan(const T &x) {
	return T(atan(F64(x)));
}

template <>
inline F32 ArcTan<F32>(const F32 &x) {
	return atanf(x);
}

template <>
inline F64 ArcTan<F64>(const F64 &x) {
	return atan(x);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::ArcTan2()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T ArcTan2(const T &y, const T &x) {
	return T(atan2(F64(y), F64(x)));
}

template <>
inline F32 ArcTan2<F32>(const F32 &y, const F32 &x) {
	return atan2f(y, x);
}

template <>
inline F64 ArcTan2<F64>(const F64 &y, const F64 &x) {
	return atan2(y, x);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::ArcSin()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T ArcSin(const T &x) {
	return T(asin(F64(x)));
}

template <>
inline F32 ArcSin<F32>(const F32 &x) {
	return asinf(x);
}

template <>
inline F64 ArcSin<F64>(const F64 &x) {
	return asin(x);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::ArcCos()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T ArcCos(const T &x) {
	return T(acos(F64(x)));
}

template <>
inline F32 ArcCos<F32>(const F32 &x) {
	return acosf(x);
}

template <>
inline F64 ArcCos<F64>(const F64 &x) {
	return acos(x);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::SquareRoot()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T SquareRoot(const T &n) {
	return T(sqrt(F64(n)));
}

template <>
inline F32 SquareRoot<F32>(const F32 &n) {
	return sqrtf(n);
}

template <>
inline F64 SquareRoot<F64>(const F64 &n) {
	return sqrt(n);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Square()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T Square(const T &n) {
	return (n * n);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Pow()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T Pow(const T &x, const T &y) {
	return T(pow(F64(x), F64(y)));
}

template <>
inline F32 Pow<F32>(const F32 &x, const F32 &y) {
	return powf(x, y);
}

template <>
inline F64 Pow<F64>(const F64 &x, const F64 &y) {
	return pow(x, y);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::IsPow2()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline bool IsPow2(const T &n) {
	RAD_STATIC_ASSERT(meta::IsInteger<T>::VALUE);
	return ((n > 0) && (0 == (n & (n - T(1)))));
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Ln()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T Ln(const T &n) {
	return T(Ln(F64(n)));
}

template <>
inline F32 Ln(const F32 &n) {
	return logf(n);
}

template <>
inline F64 Ln(const F64 &n) {
	return log(n);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Log10()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T Log10(const T &n) {
	return T(Log10(F64(n)));
}

template <>
inline F32 Log10(const F32 &n) {
	return log10f(n);
}

template <>
inline F64 Log10(const F64 &n) {
	return log10(n);
}

template <typename T>
inline T Log(const T &n, const T &base) {
	return (Ln(n) / Ln(base));
}

template <typename T, class S>
inline T Lerp(const T &start, const T &finish, const S &t) {
	return start + (finish - start) * t;
}

template <typename T>
inline T Mid(const T &a, const T &b) {
	return ((a + b) / T(2.0));
}

template <typename T>
inline T Sign(const T& a) {
	return (a > T(0)) ? T(1) : (a < T(0)) ? T(-1) : 0;
}

template <typename T>
inline int SignBits(const T& a) {
	return (a > T(0)) ? SignPosOne : (a < T(0)) ? SignNegOne : SignZero;
}

} // math

#include "../PopSystemMacros.h"

