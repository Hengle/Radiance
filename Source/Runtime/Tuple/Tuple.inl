// Tuple.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<TTail, THead>::Tuple()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTail, typename THead>
inline Tuple<TTail, THead>::Tuple(const TTail &tail, const THead &head) :
m_tail(tail),
m_head(head)
{
}

template <typename TTail, typename THead>
inline Tuple<TTail, THead>::Tuple(const SelfType &t) :
m_tail(t.m_tail),
m_head(t.m_head)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<TTail, THead>::operator +<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTail, typename THead>
template <typename T>
inline typename Tuple<TTail, THead>::template AddTraits<T>::Type Tuple<TTail, THead>::operator +(const T &head) const
{
	return typename AddTraits<T>::Type(*this, head);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<TTail, THead>::Head()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTail, typename THead>
inline THead &Tuple<TTail, THead>::Head()
{
	return m_head;
}

template <typename TTail, typename THead>
inline const THead &Tuple<TTail, THead>::Head() const
{
	return m_head;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<TTail, THead>::Tail()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTail, typename THead>
inline TTail &Tuple<TTail, THead>::Tail()
{
	return m_tail;
}

template <typename TTail, typename THead>
inline const TTail &Tuple<TTail, THead>::Tail() const
{
	return m_tail;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<TTail, THead>::Element<Index>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTail, typename THead>
template <int Index>
inline typename Tuple<TTail, THead>::template ElementTraits<Index>::Type &Tuple<TTail, THead>::Element()
{
	return TupleElement<SelfType, Index>::Get(*this);
}

template <typename TTail, typename THead>
template <int Index>
inline typename Tuple<TTail, THead>::template ElementTraits<Index>::ConstType &Tuple<TTail, THead>::Element() const
{
	return TupleElement<SelfType, Index>::Get(*this);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<void, THead>::Tuple()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename THead>
inline Tuple<void, THead>::Tuple(const THead &head) :
m_head(head)
{
}

template <typename THead>
inline Tuple<void, THead>::Tuple(const SelfType &t) :
m_head(t.m_head)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<void, THead>::operator +<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename THead>
template <typename T>
inline typename Tuple<void, THead>::template AddTraits<T>::Type Tuple<void, THead>::operator +(const T &head) const
{
	return typename AddTraits<T>::Type(*this, head);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<void, THead>::Tail()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename THead>
inline Tuple<void, void> &Tuple<void, THead>::Tail()
{
	static TailType tail;
	return tail;
}

template <typename THead>
inline const Tuple<void, void> &Tuple<void, THead>::Tail() const
{
	static const TailType tail;
	return tail;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<void, THead>::Head()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename THead>
inline THead &Tuple<void, THead>::Head()
{
	return m_head;
}

template <typename THead>
inline const THead &Tuple<void, THead>::Head() const
{
	return m_head;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<void, THead>::Element<Index>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename THead>
template <int Index>
inline typename Tuple<void, THead>::template ElementTraits<Index>::Type &Tuple<void, THead>::Element()
{
	return TupleElement<SelfType, Index>::Get(*this);
}

template <typename THead>
template <int Index>
inline typename Tuple<void, THead>::template ElementTraits<Index>::ConstType &Tuple<void, THead>::Element() const
{
	return TupleElement<SelfType, Index>::Get(*this);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<void, void>::Tuple()
//////////////////////////////////////////////////////////////////////////////////////////

inline Tuple<void, void>::Tuple()
{
}

inline Tuple<void, void>::Tuple(const SelfType &t)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<void, void>::operator +<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline typename Tuple<void, void>::AddTraits<T>::Type Tuple<void, void>::operator +(const T &head) const
{
	return typename AddTraits<T>::Type(head);
}


