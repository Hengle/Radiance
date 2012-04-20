// Utils.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <algorithm>



// Safely copy containers that contain pointers.

template<typename DstContainerType, typename SrcContainerType, typename AllocatorCallbackType>
inline void STLContainerCopy(DstContainerType& dst, const SrcContainerType& src, AllocatorCallbackType& alc)
{
	dst.clear();
	STLContainerAppend(dst, src, alc);
}

// Safely copy containers that contain pointers.

template<typename ContainerType, typename IteratorType, typename AllocatorCallbackType>
inline void STLContainerCopy(ContainerType& dst, const IteratorType& first, const IteratorType& last, AllocatorCallbackType& alc)
{
	dst.clear();
	STLContainerAppend(dst, first, last, alc);
}

// Safely append containers that contain pointers.

template<typename DstContainerType, typename SrcContainerType, typename AllocatorCallbackType>
inline void STLContainerAppend(DstContainerType& dst, const SrcContainerType& src, AllocatorCallbackType& alc)
{
	STLContainerAppend(dst, src.begin(), src.end(), alc);
}

// Safely append containers that contain pointers.

template<typename ContainerType, typename IteratorType, typename AllocatorCallbackType>
void STLContainerAppend(ContainerType& dst, const IteratorType& first, const IteratorType& last, AllocatorCallbackType& alc)
{
	struct _x
	{
		_x(ContainerType& c, AllocatorCallbackType& a) : dst(c), alc(a) {}

		void operator () (typename std::iterator_traits<IteratorType>::value_type v)
		{
			dst.push_back(alc(v));
		}

		ContainerType& dst;
		AllocatorCallbackType& alc;
	};

	std::for_each(first, last, _x(dst, alc));
}

// Safely free containers that contain pointers.

template<typename ContainerType, typename DeallocatorCallbackType>
void STLContainerFree(ContainerType& src, DeallocatorCallbackType& del)
{
	std::for_each(src.begin(), src.end(), del);
	src.clear();
}

// Frees any excess memory used by container (so allocated memory matches the number of elements).

template<typename ContainerType>
inline void STLContainerShrinkToSize(ContainerType& src)
{
	if (src.capacity() > src.size())
	{
		ContainerType temp(src);
		temp.swap(src);
	}
}

// Replace (dst_start -> dst_end) with (src_start -> src_end) in container c. The container
// is grown or shrunk as necessary.

template<typename ContainerType, typename Iter1Type, typename Iter2Type>
void _STLRangeReplace2(ContainerType& dst, Iter1Type dst_start, Iter1Type dst_end, Iter2Type src_start, Iter2Type src_end, std::random_access_iterator_tag, std::random_access_iterator_tag)
{
	// random access iterator version.
	size_t dstLen = (size_t)std::distance(dst_start, dst_end);
	size_t srcLen = (size_t)std::distance(src_start, src_end);

	if (srcLen == 0)
	{
		dst.erase(dst_start, dst_end);
	}
	else
	{
		if (dstLen == 0)
		{
			if (dst.size() == 0)
			{
				dst.assign(src_start, src_end);
			}
			else
			{
				dst.insert(dst_start, src_start, src_end);
			}
		}
		else
		{
			size_t min = std::min(srcLen, dstLen);
			Iter2Type my_src_end = src_start + min;

			std::copy(src_start, my_src_end, dst_start);

			if (dstLen > srcLen)
			{
				// erase.
				dst.erase(dst_start+min, dst_end);
			}
			else if (dstLen < srcLen)
			{
				dst.insert(dst_end, my_src_end, src_end);
			}
		}
	}
}

template<typename ContainerType, typename Iter1Type, typename Iter2Type>
inline void STLRangeReplace(ContainerType& dst, Iter1Type dst_start, Iter1Type dst_end, Iter2Type src_start, Iter2Type src_end)
{
	_STLRangeReplace2(dst, dst_start, dst_end, src_start, src_end, std::iterator_traits<Iter1Type>::iterator_category(), typename std::iterator_traits<Iter2Type>::iterator_category());
}


