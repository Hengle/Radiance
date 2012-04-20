// Math.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy

#pragma once

#include "IntMath.h"
#include "../PushPack.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math constants
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct Constants
{
	static const T PI() { return T(3.1415926535897932384626433832795); }
	static const T PI_OVER_2() { return T(1.5707963267948966192313216916395); }
};

//////////////////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T RadToDeg(const T &rad);

template <typename T>
T DegToRad(const T &deg);

template <typename T>
T Epsilon();

template <typename T>
T Abs(const T &n);

template <typename T>
bool BitScanForward(T &bit, const T &n);

template <typename T>
bool BitScanReverse(T &bit, const T &n);

template <typename T>
const T &Clamp(const T &n, const T &min, const T &max);

template <typename T>
const T &Min(const T &a, const T &b);

template <typename T>
const T &Max(const T &a, const T &b);

template <typename T>
T Floor(const T &n);

template <typename T>
T Ceil(const T &n);

template <typename T>
T Round(const T &x);

template <typename T>
bool NearlyEquals(const T &a, const T &b, const T &tolerance = Epsilon<T>());

template <typename T>
bool NearlyZero(const T &n, const T &tolerance = Epsilon<T>());

template <typename T1, class T2>
T1 Quantize(const T2 &n);

F32 Reciprocal(const F32 &n);
F64 Reciprocal(const F64 &n);
F32 Reciprocal(const int &n);

template <typename T>
T Mod(const T &x, const T &y);

template <typename T>
void ModF(T &wholePart, T &fracPart, const T &n);

template <typename T>
void SinAndCos(T *pSin, T *pCos, const T &angle);

template <typename T>
T Sin(const T &angle);

template <typename T>
T Cos(const T &angle);

template <typename T>
T Tan(const T &angle);

template <typename T>
T ArcTan(const T &x);

template <typename T>
T ArcTan2(const T &y, const T &x);

template <typename T>
T ArcSin(const T &x);

template <typename T>
T ArcCos(const T &x);

template <typename T>
T SquareRoot(const T &n);

template <typename T>
T Square(const T &n);

template <typename T>
T Pow(const T &x, const T &y);

template <typename T>
bool IsPow2(const T &n);

// Natural log

template <typename T>
T Ln(const T &n);

template <typename T>
T Log10(const T &n);

template <typename T>
T Log(const T &n, const T &base);

template <typename T, class S>
T Lerp(const T &start, const T &finish, const S &t);

template <typename T>
T Mid(const T &a, const T &b);

// (a>0) ? 1 : (a<0) ? -1 : 0

template <typename T>
T Sign(const T& a);

// (a>0) ? SignPosOne : (a<0) ? SignNegOne : SignZero

enum
{
	SignZero,
	RAD_FLAG(SignPosOne),
	RAD_FLAG(SignNegOne)
};

template <typename T>
int SignBits(const T& a);

} // math


#include "../PopPack.h"
#include "Math.inl"
