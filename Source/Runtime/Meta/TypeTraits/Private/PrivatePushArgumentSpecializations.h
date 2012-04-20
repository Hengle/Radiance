// PrivatePushArgumentSpecializations.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL ()>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct PushArgument<R RADMETA_CALLDECL (), void>
{
	typedef R RADMETA_CALLDECL Type ();
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (), A0>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename R, typename A0>
struct PushArgument<R RADMETA_CALLDECL (), A0>
{
	typedef R RADMETA_CALLDECL Type (A0);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0), A1>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename R, typename A0, typename A1>
struct PushArgument<R RADMETA_CALLDECL (A0), A1>
{
	typedef R RADMETA_CALLDECL Type (A0, A1);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1), A2>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename R, typename A0, typename A1, typename A2>
struct PushArgument<R RADMETA_CALLDECL (A0, A1), A2>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2), A3>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3
>
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2), A3>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3), A4>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4
>
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3), A4>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4), A5>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4), A5>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5), A6>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5), A6>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6), A7>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6), A7>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7), A8>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7), A8>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8), A9>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8), A9>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9), A10>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9), A10>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10), A11>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10), A11>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11), A12>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11), A12>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12), A13>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12), A13>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13), A14>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13), A14>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14), A15>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14), A15>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15), A16>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15), A16>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16), A17>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16), A17>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17), A18>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17), A18>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18), A19>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18), A19>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19);
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19), A20>
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
struct PushArgument<R RADMETA_CALLDECL (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19), A20>
{
	typedef R RADMETA_CALLDECL Type (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20);
};
