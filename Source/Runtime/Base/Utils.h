// Utils.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once
#include <stdlib.h>
#include <limits>

typedef char SizeBuffer[32];

RADRT_API void RADRT_CALL FormatSize(SizeBuffer &buffer, AddrSize size);


inline int FloatToInt(float x)
{ 
	// decode [sgn+1][exp+8][mantissa+23]
	unsigned int &bits = *reinterpret_cast<unsigned int*>(&x);
	unsigned int m = 0x80000000 | (bits << 8);
	unsigned int e = (127+31) - ((bits & 0x7F800000) >> 23);
	int z = (m>>e) & -(e<32);
	if (bits & 0x80000000)
		return -z;
	return z;
}

inline int FloorFastInt(float x)
{
	return FloatToInt(x);
}

inline int CeilFastInt(float x)
{
	return FloatToInt(x+1.f) - 1;
}

inline float FloorFastFloat(float x)
{
	return (float)FloorFastInt(x);
}

inline float CeilFastFloat(float x)
{
	return (float)CeilFastInt(x);
}

// returns the index of the lowest set bit (i.e. 1-32). if zero is returned no bits are set.

template <typename T>
inline T LowBit(T val)
{
	if (!val) 
		return 0;

	T c = 0;
	T l = 0;
	T v = 1;

	while (v > l)
	{
		if (val & v)
			return c;
		++c;
		l = v;
		v <<= 1;
	}

	return 0;
}

// returns the # of bits set.

template <typename T>
inline T NumBits(T val)
{
	T c = 0;
	T l = 0;
	T v = 1;

	while (v > l)
	{
		if (val & v) 
			++c;
		l = v;
		v <<= 1;
	}

	return c;
}

template <typename T>
inline bool IsPowerOf2(const T &x)
{
	return (x & (x-1)) == 0;
}

template <typename T>
inline T PowerOf2(T val)
{
	// http://jeffreystedfast.blogspot.com/2008/06/calculating-nearest-power-of-2.html

	if ((val&(val-1)) == 0)
		return val;
	T j, k;
    (j = val & 0xFFFF0000) || (j = val);
    (k = j & 0xFF00FF00) || (k = j);
    (j = k & 0xF0F0F0F0) || (j = k);
    (k = j & 0xCCCCCCCC) || (k = j);
    (j = k & 0xAAAAAAAA) || (j = k);
    return j << 1;
}

template <typename T>
inline T HighBit(T val)
{
	if (!val)
		return T(0);
	// clear all but high bit.
	val = IsPowerOf2(val) ? val : (PowerOf2(val)>>1);
	return LowBit(val);
}

template <typename T>
inline T LowBitVal(T val)
{
	if (!val)
		return T(0);
	return 1<<LowBit(val);
}

template <typename T>
inline T HighBitVal(T val)
{
	if (!val)
		return T(0);
	return 1<<HighBit(val);
}

template< typename T >
inline T Clamp(const T& val, const T& min, const T& max)
{
	return (val < min) ? min : (val > max) ? max : val;
}

// Safely copy containers that contain pointers by creating new objects and copying
// them.

template<typename DstContainerType, typename SrcContainerType, typename AllocatorCallbackType>
void STLContainerCopy(DstContainerType& dst, const SrcContainerType& src, AllocatorCallbackType& alc);

// Safely copy containers that contain pointers by creating new objects and copying
// them.

template<typename ContainerType, typename IteratorType, typename AllocatorCallbackType>
void STLContainerCopy(ContainerType& dst, const IteratorType& first, const IteratorType& last, AllocatorCallbackType& alc);

// Safely append containers that contain pointers by creating new objects and copying
// them.

template<typename DstContainerType, typename SrcContainerType, typename AllocatorCallbackType>
void STLContainerAppend(DstContainerType& dst, const SrcContainerType& src, AllocatorCallbackType& alc);

// Safely append containers that contain pointers by creating new objects and copying
// them.

template<typename ContainerType, typename IteratorType, typename AllocatorCallbackType>
void STLContainerAppend(ContainerType& dst, const IteratorType& first, const IteratorType& last, AllocatorCallbackType& alc);

// Destruct pointers in container, and clear().

template<typename ContainerType, typename DeallocatorCallbackType>
void STLContainerFree(ContainerType& src, DeallocatorCallbackType& alc);

// Frees any excess memory used by container (so allocated memory matches the number of elements).

template<typename ContainerType>
void STLContainerShrinkToSize(ContainerType& src);

// Replace (dst_start -> dst_end) with (src_start -> src_end) in container c. The container
// is grown or shrunk as necessary.

template<typename ContainerType, typename Iter1Type, typename Iter2Type>
void STLRangeReplace(ContainerType& dst, Iter1Type dst_start, Iter1Type dst_end, Iter2Type src_start, Iter2Type src_end);

// message box
#if defined(RAD_OPT_PC)
enum MessageBoxButton
{
	MBOk,
	MBCancel,
	MBYes,
	MBNo
};

enum MessageBoxStyle
{
	MBStyleOk,
	MBStyleOkCancel,
	MBStyleYesNo
};

RADRT_API int RADRT_CALL MessageBox(const char *title, const char *message, MessageBoxStyle style);
#endif

#include "Utils.inl"



