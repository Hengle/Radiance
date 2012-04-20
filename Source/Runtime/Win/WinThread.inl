// WinThread.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// 32 bit intrinsics
//////////////////////////////////////////////////////////////////////////////////////////

extern "C" long __cdecl _InterlockedIncrement(long volatile*);
extern "C" long __cdecl _InterlockedDecrement(long volatile*);
extern "C" long __cdecl _InterlockedExchange(long volatile*, long);
extern "C" long __cdecl _InterlockedExchangeAdd(long volatile*, long);
extern "C" long __cdecl _InterlockedAnd(long volatile*, long);
extern "C" long __cdecl _InterlockedOr(long volatile*, long);
extern "C" long __cdecl _InterlockedXor(long volatile*, long);

#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedAnd)
#pragma intrinsic(_InterlockedOr)
#pragma intrinsic(_InterlockedXor)

//////////////////////////////////////////////////////////////////////////////////////////
// 64 bit intrinsics
//////////////////////////////////////////////////////////////////////////////////////////

#if RAD_OPT_MACHINE_WORD_SIZE == 8

extern "C" long __cdecl _InterlockedIncrement64(__int64 volatile*);
extern "C" long __cdecl _InterlockedDecrement64(__int64 volatile*);
extern "C" long __cdecl _InterlockedExchange64(__int64 volatile*, __int64);
extern "C" long __cdecl _InterlockedExchangeAdd64(__int64 volatile*, __int64);
extern "C" long __cdecl _InterlockedAnd64(__int64 volatile*, __int64);
extern "C" long __cdecl _InterlockedOr64(__int64 volatile*, __int64);
extern "C" long __cdecl _InterlockedXor64(__int64 volatile*, __int64);

#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedAnd64)
#pragma intrinsic(_InterlockedOr64)
#pragma intrinsic(_InterlockedXor64)

#endif


namespace thread {
namespace details {

template<typename T>
inline Interlocked<T>::Interlocked()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// thread::details::Interlocked<S32>
//////////////////////////////////////////////////////////////////////////////////////////

template<>
inline Interlocked<S32>::Interlocked()
{
}

template<>
inline Interlocked<S32>::Interlocked(const Interlocked<S32>& s) :
AlignedHelper<S32>(s)
{
}

template<>
inline Interlocked<S32>::Interlocked(const S32& val) :
AlignedHelper<S32>(val)
{
}

template<>
inline Interlocked<S32>::~Interlocked()
{
}

template<>
inline Interlocked<S32>& Interlocked<S32>::operator = (const S32& val)
{
	::_InterlockedExchange(reinterpret_cast<long volatile*>(&m_val), val);
	return *this;
}

template<>
inline Interlocked<S32>::operator S32 () const
{
	return m_val;
}

template<>
inline S32 Interlocked<S32>::operator ++ () // prefix
{
	return (S32)::_InterlockedIncrement(reinterpret_cast<long volatile*>(&m_val));
}

template<>
inline S32 Interlocked<S32>::operator ++ (int) // postfix
{
	return (S32)::_InterlockedExchangeAdd(reinterpret_cast<long volatile*>(&m_val), 1);
}

template<>
inline S32 Interlocked<S32>::operator -- () // prefix
{
	return (S32)::_InterlockedDecrement(reinterpret_cast<long volatile*>(&m_val));
}

template<>
inline S32 Interlocked<S32>::operator -- (int) // postfix
{
	return (S32)::_InterlockedExchangeAdd(reinterpret_cast<long volatile*>(&m_val), -1);
}

template<>
inline S32 Interlocked<S32>::operator += (const S32& x)
{
	return (S32)::_InterlockedExchangeAdd(reinterpret_cast<long volatile*>(&m_val), x) + x;
}

template<>
inline S32 Interlocked<S32>::operator -= (const S32& x)
{
	return (S32)::_InterlockedExchangeAdd(reinterpret_cast<long volatile*>(&m_val), -x) - x;
}

template<>
inline S32 Interlocked<S32>::operator |= (const S32& x)
{
	return (S32)::_InterlockedOr(reinterpret_cast<long volatile*>(&m_val), x) | x;
}

template<>
inline S32 Interlocked<S32>::operator &= (const S32& x)
{
	return (S32)::_InterlockedAnd(reinterpret_cast<long volatile*>(&m_val), x) & x;
}

template<>
inline S32 Interlocked<S32>::operator ^= (const S32& x)
{
	return (S32)::_InterlockedXor(reinterpret_cast<long volatile*>(&m_val), x) ^ x;
}

//////////////////////////////////////////////////////////////////////////////////////////
// thread::details::Interlocked<U32>
//////////////////////////////////////////////////////////////////////////////////////////

template<>
inline Interlocked<U32>::Interlocked()
{
}

template<>
inline Interlocked<U32>::Interlocked(const Interlocked<U32> &s) :
AlignedHelper<U32>(s)
{
}

template<>
inline Interlocked<U32>::Interlocked(const U32 &val) :
AlignedHelper<U32>(val)
{
}

template<>
inline Interlocked<U32>::~Interlocked()
{
}

template<>
inline Interlocked<U32>& Interlocked<U32>::operator = (const U32& val)
{
	::_InterlockedExchange(reinterpret_cast<long volatile*>(&m_val), val);
	return *this;
}

template<>
inline Interlocked<U32>::operator U32 () const
{
	return m_val;
}

template<>
inline U32 Interlocked<U32>::operator ++ () // prefix
{
	return (U32)::_InterlockedIncrement(reinterpret_cast<long volatile*>(&m_val));
}

template<>
inline U32 Interlocked<U32>::operator ++ (int) // postfix
{
	return (U32)::_InterlockedExchangeAdd(reinterpret_cast<long volatile*>(&m_val), 1);
}

template<>
inline U32 Interlocked<U32>::operator -- () // prefix
{
	return (U32)::_InterlockedDecrement(reinterpret_cast<long volatile*>(&m_val));
}

template<>
inline U32 Interlocked<U32>::operator -- (int) // postfix
{
	return (U32)::_InterlockedExchangeAdd(reinterpret_cast<long volatile*>(&m_val), -1);
}

template<>
inline U32 Interlocked<U32>::operator += (const U32& x)
{
	return (U32)::_InterlockedExchangeAdd(reinterpret_cast<long volatile*>(&m_val), x) + x;
}

template<>
inline U32 Interlocked<U32>::operator -= (const U32& x)
{
	return (U32)::_InterlockedExchangeAdd(reinterpret_cast<long volatile*>(&m_val), -((S32)x)) - x;
}

template<>
inline U32 Interlocked<U32>::operator |= (const U32& x)
{
	return (U32)::_InterlockedOr(reinterpret_cast<long volatile*>(&m_val), x) | x;
}

template<>
inline U32 Interlocked<U32>::operator &= (const U32& x)
{
	return (U32)::_InterlockedAnd(reinterpret_cast<long volatile*>(&m_val), x) & x;
}

template<>
inline U32 Interlocked<U32>::operator ^= (const U32& x)
{
	return (U32)::_InterlockedXor(reinterpret_cast<long volatile*>(&m_val), x) ^ x;
}

#if RAD_OPT_MACHINE_WORD_SIZE == 8

//////////////////////////////////////////////////////////////////////////////////////////
// thread::details::Interlocked<S64>
//////////////////////////////////////////////////////////////////////////////////////////

template<>
inline Interlocked<S64>::Interlocked()
{
}

template<>
inline Interlocked<S64>::Interlocked(const Interlocked<S64> &s) :
AlignedHelper<S64>(s)
{
}

template<>
inline Interlocked<S64>::Interlocked(const S64 &val) :
AlignedHelper<S64>(val)
{
}

template<>
inline Interlocked<S64>::~Interlocked()
{
}

template<>
inline Interlocked<S64>& Interlocked<S64>::operator = (const S64 &val)
{
	::_InterlockedExchange64(reinterpret_cast<__int64 volatile*>(&m_val), val);
	return *this;
}

template<>
inline Interlocked<S64>::operator S64 () const
{
	return m_val;
}

template<>
inline S64 Interlocked<S64>::operator ++ () // prefix
{
	return (S64)::_InterlockedIncrement64(reinterpret_cast<__int64 volatile*>(&m_val));
}

template<>
inline S64 Interlocked<S64>::operator ++ (int) // postfix
{
	return (S64)::_InterlockedExchangeAdd64(reinterpret_cast<__int64 volatile*>(&m_val), 1);
}

template<>
inline S64 Interlocked<S64>::operator -- () // prefix
{
	return (S64)::_InterlockedDecrement64(reinterpret_cast<__int64 volatile*>(&m_val));
}

template<>
inline S64 Interlocked<S64>::operator -- (int) // postfix
{
	return (S64)::_InterlockedExchangeAdd64(reinterpret_cast<__int64 volatile*>(&m_val), -1);
}

template<>
inline S64 Interlocked<S64>::operator += (const S64& x)
{
	return (S64)::_InterlockedExchangeAdd64(reinterpret_cast<__int64 volatile*>(&m_val), x) + x;
}

template<>
inline S64 Interlocked<S64>::operator -= (const S64& x)
{
	return (S64)::_InterlockedExchangeAdd64(reinterpret_cast<__int64 volatile*>(&m_val), -x) - x;
}

template<>
inline S64 Interlocked<S64>::operator |= (const S64& x)
{
	return (S64)::_InterlockedOr64(reinterpret_cast<__int64 volatile*>(&m_val), x) | x;
}

template<>
inline S64 Interlocked<S64>::operator &= (const S64& x)
{
	return (S64)::_InterlockedAnd64(reinterpret_cast<__int64 volatile*>(&m_val), x) & x;
}

template<>
inline S64 Interlocked<S64>::operator ^= (const S64& x)
{
	return (S64)::_InterlockedXor64(reinterpret_cast<__int64 volatile*>(&m_val), x) ^ x;
}

//////////////////////////////////////////////////////////////////////////////////////////
// thread::details::Interlocked<U64>
//////////////////////////////////////////////////////////////////////////////////////////

template<>
inline Interlocked<U64>::Interlocked()
{
}

template<>
inline Interlocked<U64>::Interlocked(const Interlocked<U64> &s) :
AlignedHelper<U64>(s)
{
}

template<>
inline Interlocked<U64>::Interlocked(const U64 &val) :
AlignedHelper<U64>(val)
{
}

template<>
inline Interlocked<U64>::~Interlocked()
{
}

template<>
inline Interlocked<U64>& Interlocked<U64>::operator = (const U64 &val)
{
	::_InterlockedExchange64(reinterpret_cast<__int64 volatile*>(&m_val), val);
	return *this;
}

template<>
inline Interlocked<U64>::operator U64 () const
{
	return m_val;
}

template<>
inline U64 Interlocked<U64>::operator ++ () // prefix
{
	return (U64)::_InterlockedIncrement64(reinterpret_cast<__int64 volatile*>(&m_val));
}

template<>
inline U64 Interlocked<U64>::operator ++ (int) // postfix
{
	return (U64)::_InterlockedExchangeAdd64(reinterpret_cast<__int64 volatile*>(&m_val), 1);
}

template<>
inline U64 Interlocked<U64>::operator -- () // prefix
{
	return (U64)::_InterlockedDecrement64(reinterpret_cast<__int64 volatile*>(&m_val));
}

template<>
inline U64 Interlocked<U64>::operator -- (int) // postfix
{
	return (U64)::_InterlockedExchangeAdd64(reinterpret_cast<__int64 volatile*>(&m_val), -1);
}

template<>
inline U64 Interlocked<U64>::operator += (const U64& x)
{
	return (U64)::_InterlockedExchangeAdd64(reinterpret_cast<__int64 volatile*>(&m_val), x) + x;
}

template<>
inline U64 Interlocked<U64>::operator -= (const U64& x)
{
	return (U64)::_InterlockedExchangeAdd64(reinterpret_cast<__int64 volatile*>(&m_val), -x) - x;
}

template<>
inline U64 Interlocked<U64>::operator |= (const U64& x)
{
	return (U64)::_InterlockedOr64(reinterpret_cast<__int64 volatile*>(&m_val), x) | x;
}

template<>
inline U64 Interlocked<U64>::operator &= (const U64& x)
{
	return (U64)::_InterlockedAnd64(reinterpret_cast<__int64 volatile*>(&m_val), x) & x;
}

template<>
inline U64 Interlocked<U64>::operator ^= (const U64& x)
{
	return (U64)::_InterlockedXor64(reinterpret_cast<__int64 volatile*>(&m_val), x) ^ x;
}

#endif // RAD_OPT_MACHINE_WORD_SIZE == 8

} // details
} // thread
