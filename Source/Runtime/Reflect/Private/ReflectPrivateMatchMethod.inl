// ReflectPrivateMatchMethod.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// Method match specializations
//////////////////////////////////////////////////////////////////////////////////////////

PRIVATE_RADREFLECT_MATCHMETHOD(0, (true
))

PRIVATE_RADREFLECT_MATCHMETHOD(1, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(2, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(3, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(4, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(5, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(6, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(7, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(8, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(9, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 8>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(10, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 8>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 9>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(11, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 8>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 9>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 10>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(12, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 8>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 9>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 10>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 11>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(13, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 8>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 9>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 10>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 11>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 12>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(14, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 8>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 9>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 10>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 11>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 12>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 13>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(15, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 8>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 9>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 10>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 11>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 12>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 13>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 14>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(16, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 8>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 9>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 10>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 11>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 12>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 13>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 14>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 15>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(17, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 8>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 9>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 10>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 11>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 12>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 13>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 14>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 15>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 16>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(18, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 8>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 9>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 10>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 11>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 12>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 13>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 14>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 15>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 16>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 17>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(19, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 8>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 9>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 10>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 11>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 12>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 13>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 14>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 15>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 16>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 17>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 18>::Match(method, args)
))

PRIVATE_RADREFLECT_MATCHMETHOD(20, (
	ArgumentMatch<TMethod, TFunction, 0>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 1>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 2>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 3>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 4>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 5>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 6>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 7>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 8>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 9>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 10>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 11>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 12>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 13>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 14>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 15>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 16>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 17>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 18>::Match(method, args) &&
	ArgumentMatch<TMethod, TFunction, 19>::Match(method, args)
))

} // details
} // reflect

