/*! \file StackVector.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup container
*/

#pragma once
#include <vector>
#include "StackContainer.h"

// Use the STACKIFY macro: STACKIFY(std::vector<int>, intVec, 1024);
template <typename T, typename A>
struct stackify_container< std::vector<T, A> > {
	typedef T ElementType;
	
	template <size_t TNumElms>
	struct allocator {
		typedef typename stack_allocator<T, TNumElms> Type;
	};

	template <size_t TNumElms>
	struct container {
		typedef std::vector<T, typename allocator<TNumElms>::Type> Type;
	};
};
