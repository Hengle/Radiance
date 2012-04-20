// PrivateFunctionTraitsSpecializations.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL ()>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct FunctionTraits<R RADMETA_CALLDECL ()> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (),
	R, void, 0
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0)>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename R, typename A0>
struct FunctionTraits<R RADMETA_CALLDECL (A0)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (),
	R, A0, 1
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1)>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename R, typename A0, typename A1>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0),
	R, A1, 2
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2)>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename R, typename A0, typename A1, typename A2>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1),
	R, A2, 3
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2),
	R, A3, 4
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3),
	R, A4, 5
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4),
	R, A5, 6
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5),
	R, A6, 7
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6),
	R, A7, 8
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7),
	R, A8, 9
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8),
	R, A9, 10
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9),
	R, A10, 11
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10),
	R, A11, 12
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11),
	R, A12, 13
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12),
	R, A13, 14
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13),
	R, A14, 15
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14),
	R, A15, 16
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15),
	R, A16, 17
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename A17
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16),
	R, A17, 18
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename A17,
	typename A18
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17),
	R, A18, 19
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename A17,
	typename A18,
	typename A19
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18),
	R, A19, 20
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20)>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename A17,
	typename A18,
	typename A19,
	typename A20
>
struct FunctionTraits<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20)> :
public details::FunctionBaseTraits<
	R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19),
	R, A20, 21
>
{
};
