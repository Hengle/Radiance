// Predicate.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.


namespace predicate {

//////////////////////////////////////////////////////////////////////////////////////////
// predicate::ImplicitCastAdapter< typename _ARG_RET_TYPE, typename _ARG_TYPE >
//////////////////////////////////////////////////////////////////////////////////////////
//
// Transform operator, takes a source type and implicitly casts it.
//
//////////////////////////////////////////////////////////////////////////////////////////

template< typename _ARG_RET_TYPE, typename _ARG_TYPE >
inline _ARG_RET_TYPE ImplicitCastAdapter< _ARG_RET_TYPE, _ARG_TYPE >::operator () (_ARG_TYPE arg) { return arg; }

//////////////////////////////////////////////////////////////////////////////////////////
// predicate::ExplicitCastAdapter< typename _ARG_RET_TYPE, typename _ARG_TYPE >
//////////////////////////////////////////////////////////////////////////////////////////
//
// Transform operator, takes a source type and explicitly casts it.
//
//////////////////////////////////////////////////////////////////////////////////////////

template< typename _ARG_RET_TYPE, typename _ARG_TYPE >
inline _ARG_RET_TYPE ExplicitCastAdapter< _ARG_RET_TYPE, _ARG_TYPE >::operator () (_ARG_TYPE arg) { return (_ARG_RET_TYPE)arg; }

//////////////////////////////////////////////////////////////////////////////////////////
// predicate::Compare< typename _ARG0_TYPE, typename _ARG1_TYPE, typename _ARG0_ADAPTER, typename _ARG1_ADAPTER >
//////////////////////////////////////////////////////////////////////////////////////////

template
<
	class    _ARG0_ADAPTER,
	class    _ARG1_ADAPTER
>
inline SReg Compare< _ARG0_ADAPTER, _ARG1_ADAPTER >::operator () (const Adapter0RetType& x, const Adapter1RetType& y)
{
	return (x < y) ? -1 : (x > y) ? 1 : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// predicate::ReverseCompare< typename _ARG0_TYPE, typename _ARG1_TYPE, typename _ARG0_ADAPTER, typename _ARG1_ADAPTER >
//////////////////////////////////////////////////////////////////////////////////////////

template
<
	class    _ARG0_ADAPTER,
	class    _ARG1_ADAPTER
>
inline SReg ReverseCompare< _ARG0_ADAPTER, _ARG1_ADAPTER >::operator () (const Adapter0RetType& x, const Adapter1RetType& y)
{
	return (x < y) ? 1 : (x > y) ? -1 : 0;
}

} // predicate

