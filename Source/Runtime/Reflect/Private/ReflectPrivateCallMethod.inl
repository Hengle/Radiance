// ReflectPrivateCallMethod.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// Method call specializations
//////////////////////////////////////////////////////////////////////////////////////////

PRIVATE_RADREFLECT_CALLMETHOD(0, (
))

PRIVATE_RADREFLECT_CALLMETHOD(1, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(2, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(3, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(4, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(5, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(6, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(7, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(8, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(9, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(10, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 9>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(11, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 9>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 10>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(12, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 9>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 10>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 11>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(13, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 9>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 10>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 11>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 12>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(14, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 9>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 10>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 11>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 12>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 13>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(15, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 9>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 10>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 11>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 12>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 13>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 14>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(16, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 9>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 10>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 11>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 12>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 13>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 14>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 15>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(17, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 9>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 10>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 11>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 12>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 13>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 14>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 15>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 16>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(18, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 9>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 10>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 11>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 12>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 13>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 14>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 15>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 16>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 17>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(19, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 9>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 10>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 11>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 12>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 13>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 14>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 15>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 16>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 17>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 18>::Value(method, args)
))

PRIVATE_RADREFLECT_CALLMETHOD(20, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 9>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 10>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 11>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 12>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 13>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 14>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 15>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 16>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 17>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 18>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 19>::Value(method, args)
))


PRIVATE_RADREFLECT_CALLMETHOD(21, (
	ArgumentExtract<TMethod, TFunction, 0>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 1>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 2>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 3>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 4>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 5>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 6>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 7>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 8>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 9>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 10>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 11>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 12>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 13>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 14>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 15>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 16>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 17>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 18>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 19>::Value(method, args),
	ArgumentExtract<TMethod, TFunction, 20>::Value(method, args)
))

} // details
} // reflect

