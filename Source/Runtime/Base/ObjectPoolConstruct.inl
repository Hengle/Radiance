// ObjectPoolConstruct.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// auto generated by Radiance/Source/object_pool_constructors.py
// See Radiance/LICENSE for licensing terms.

template <typename A1>
inline T *Construct(A1 &a1)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1);
}

template <typename A1>
inline T *SafeConstruct(A1 &a1)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1);
}

template <typename A1>
inline T *Construct(const A1 &a1)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1);
}

template <typename A1>
inline T *SafeConstruct(const A1 &a1)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1);
}

template <typename A1>
inline T *Construct(volatile A1 &a1)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1);
}

template <typename A1>
inline T *SafeConstruct(volatile A1 &a1)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1);
}

template <typename A1>
inline T *Construct(const volatile A1 &a1)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1);
}

template <typename A1>
inline T *SafeConstruct(const volatile A1 &a1)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1);
}

template <typename A1, typename A2>
inline T *Construct(A1 &a1, A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(A1 &a1, A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(A1 &a1, const A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(A1 &a1, const A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(A1 &a1, volatile A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(A1 &a1, volatile A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(A1 &a1, const volatile A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(A1 &a1, const volatile A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(const A1 &a1, A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(const A1 &a1, A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(const A1 &a1, const A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(const A1 &a1, const A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(const A1 &a1, volatile A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(const A1 &a1, volatile A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(const A1 &a1, const volatile A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(const A1 &a1, const volatile A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(volatile A1 &a1, A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(volatile A1 &a1, A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(volatile A1 &a1, const A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(volatile A1 &a1, const A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(volatile A1 &a1, volatile A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(volatile A1 &a1, volatile A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(volatile A1 &a1, const volatile A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(volatile A1 &a1, const volatile A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(const volatile A1 &a1, A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(const volatile A1 &a1, A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(const volatile A1 &a1, const A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(const volatile A1 &a1, const A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(const volatile A1 &a1, volatile A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(const volatile A1 &a1, volatile A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2>
inline T *Construct(const volatile A1 &a1, const volatile A2 &a2)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2);
}

template <typename A1, typename A2>
inline T *SafeConstruct(const volatile A1 &a1, const volatile A2 &a2)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, const A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, const A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, const A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, const A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, const A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, const A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, const A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, const A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, volatile A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, volatile A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, volatile A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, volatile A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, volatile A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, volatile A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, volatile A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, volatile A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, const volatile A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, const volatile A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, const volatile A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, const volatile A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, const volatile A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, const volatile A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(A1 &a1, const volatile A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(A1 &a1, const volatile A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, const A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, const A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, const A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, const A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, const A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, const A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, const A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, const A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, volatile A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, volatile A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, volatile A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, volatile A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, volatile A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, volatile A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, volatile A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, volatile A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, const volatile A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, const volatile A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, const volatile A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, const volatile A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, const volatile A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, const volatile A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const A1 &a1, const volatile A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const A1 &a1, const volatile A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, const A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, const A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, const A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, const A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, const A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, const A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, const A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, const A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, volatile A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, volatile A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, volatile A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, volatile A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, volatile A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, volatile A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, volatile A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, volatile A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, const volatile A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, const volatile A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, const volatile A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, const volatile A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, const volatile A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, const volatile A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(volatile A1 &a1, const volatile A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(volatile A1 &a1, const volatile A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, const A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, const A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, const A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, const A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, const A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, const A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, const A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, const A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, volatile A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, volatile A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, volatile A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, volatile A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, volatile A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, volatile A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, volatile A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, volatile A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, const volatile A2 &a2, A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, const volatile A2 &a2, A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, const volatile A2 &a2, const A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, const volatile A2 &a2, const A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, const volatile A2 &a2, volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, const volatile A2 &a2, volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *Construct(const volatile A1 &a1, const volatile A2 &a2, const volatile A3 &a3)
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
	return 0;
	return new (p) T(a1, a2, a3);
}

template <typename A1, typename A2, typename A3>
inline T *SafeConstruct(const volatile A1 &a1, const volatile A2 &a2, const volatile A3 &a3)
{
	return new (m_memoryPool.SafeGetChunk()) T(a1, a2, a3);
}

