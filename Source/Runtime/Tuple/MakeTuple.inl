// MakeTuple.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// MakeTuple()
//////////////////////////////////////////////////////////////////////////////////////////

inline TupleTraits<>::Type MakeTuple()
{
	return TupleTraits<>::Type();
}

//////////////////////////////////////////////////////////////////////////////////////////
// MakeTuple<T0>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T0>
inline typename TupleTraits<T0>::Type MakeTuple(const T0 &t0)
{
	return typename TupleTraits<T0>::Type(t0);
}

//////////////////////////////////////////////////////////////////////////////////////////
// MakeTuple<T0, T1>()
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1
>
inline typename TupleTraits<T0, T1>::Type MakeTuple(
	const T0 &t0,
	const T1 &t1
)
{
	return typename TupleTraits<T0, T1>::Type(
		MakeTuple(t0),
		t1
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// MakeTuple<T0, T1, T2>()
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2
>
inline typename TupleTraits<T0, T1, T2>::Type MakeTuple(
	const T0 &t0,
	const T1 &t1,
	const T2 &t2
)
{
	return typename TupleTraits<T0, T1, T2>::Type(
		MakeTuple(t0, t1),
		t2
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// MakeTuple<T0, T1, T2, T3>()
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3
>
inline typename TupleTraits<T0, T1, T2, T3>::Type MakeTuple(
	const T0 &t0,
	const T1 &t1,
	const T2 &t2,
	const T3 &t3
)
{
	return typename TupleTraits<T0, T1, T2, T3>::Type(
		MakeTuple(t0, t1, t2),
		t3
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// MakeTuple<T0, T1, T2, T3, T4>()
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3,
	typename T4
>
inline typename TupleTraits<T0, T1, T2, T3, T4>::Type MakeTuple(
	const T0 &t0,
	const T1 &t1,
	const T2 &t2,
	const T3 &t3,
	const T4 &t4
)
{
	return typename TupleTraits<T0, T1, T2, T3, T4>::Type(
		MakeTuple(t0, t1, t2, t3),
		t4
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// MakeTuple<T0, T1, T2, T3, T4, T5>()
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5
>
inline typename TupleTraits<T0, T1, T2, T3, T4, T5>::Type MakeTuple(
	const T0 &t0,
	const T1 &t1,
	const T2 &t2,
	const T3 &t3,
	const T4 &t4,
	const T5 &t5
)
{
	return typename TupleTraits<T0, T1, T2, T3, T4, T5>::Type(
		MakeTuple(t0, t1, t2, t3, t4),
		t5
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// MakeTuple<T0, T1, T2, T3, T4, T5, T6>()
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6
>
inline typename TupleTraits<T0, T1, T2, T3, T4, T5, T6>::Type MakeTuple(
	const T0 &t0,
	const T1 &t1,
	const T2 &t2,
	const T3 &t3,
	const T4 &t4,
	const T5 &t5,
	const T6 &t6
)
{
	return typename TupleTraits<T0, T1, T2, T3, T4, T5, T6>::Type(
		MakeTuple(t0, t1, t2, t3, t4, t5),
		t6
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// MakeTuple<T0, T1, T2, T3, T4, T5, T6, T7>()
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7
>
inline typename TupleTraits<T0, T1, T2, T3, T4, T5, T6, T7>::Type MakeTuple(
	const T0 &t0,
	const T1 &t1,
	const T2 &t2,
	const T3 &t3,
	const T4 &t4,
	const T5 &t5,
	const T6 &t6,
	const T7 &t7
)
{
	return typename TupleTraits<T0, T1, T2, T3, T4, T5, T6, T7>::Type(
		MakeTuple(t0, t1, t2, t3, t4, t5, t6),
		t7
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// MakeTuple<T0, T1, T2, T3, T4, T5, T6, T7, T8>()
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8
>
inline typename TupleTraits<T0, T1, T2, T3, T4, T5, T6, T7, T8>::Type MakeTuple(
	const T0 &t0,
	const T1 &t1,
	const T2 &t2,
	const T3 &t3,
	const T4 &t4,
	const T5 &t5,
	const T6 &t6,
	const T7 &t7,
	const T8 &t8
)
{
	return typename TupleTraits<T0, T1, T2, T3, T4, T5, T6, T7, T8>::Type(
		MakeTuple(t0, t1, t2, t3, t4, t5, t6, t7),
		t8
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// MakeTuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>()
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8,
	typename T9
>
inline typename TupleTraits<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>::Type MakeTuple(
	const T0 &t0,
	const T1 &t1,
	const T2 &t2,
	const T3 &t3,
	const T4 &t4,
	const T5 &t5,
	const T6 &t6,
	const T7 &t7,
	const T8 &t8,
	const T9 &t9
)
{
	return typename TupleTraits<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>::Type(
		MakeTuple(t0, t1, t2, t3, t4, t5, t6, t7, t8),
		t9
	);
}


