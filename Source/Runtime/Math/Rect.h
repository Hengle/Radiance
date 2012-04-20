// Rect.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "RectDef.h"
#include "Vector.h"
#include "../PushPack.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math::Rect
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Rect
{
public:

	Rect();
	Rect(T left, T top, T right, T bottom);
	Rect(const Vector2<T> &mins, const Vector2<T> &maxs);

	~Rect();

	// Mutators

	void Initialize(T left, T top, T right, T bottom);
	void Initialize(const Vector2<T> &mins, const Vector2<T> &maxs);

	void SetMins(const Vector2<T> &mins);
	void SetMaxs(const Vector2<T> &maxs);
	void Expand(const Vector2<T> &exp);

	void SetLeft(T left);
	void SetTop(T top);
	void SetRight(T right);
	void SetBottom(T bottom);

	// Accessors

	const Vector2<T> &Mins() const;
	Vector2<T> &Mins();

	const Vector2<T> &Maxs() const;
	Vector2<T> &Maxs();

	Vector2<T> Center() const;
	Vector2<T> Extents() const;

	T Left() const;
	T Top() const;
	T Right() const;
	T Bottom() const;

	T Width() const;
	T Height() const;

	// Hints

	enum Hint
	{
		NOT_DEGENERATE
	};

	// Queries

	bool IsDegenerate() const;

	bool ContainsX(T x) const;
	bool ContainsY(T y) const;
	bool Contains(const Vector2<T>& point) const;
	bool Contains(const Rect& rect) const;

	bool Intersects(const Rect& rect, Hint hint) const;
	bool Intersects(const Rect& rect) const;

	T Area() const;

	// Intersection

	template <typename X>
	friend Rect Intersection(const Rect &a, const Rect &b, Hint hint);

	template <typename X>
	friend Rect Intersection(const Rect &a, const Rect &b);

	Rect operator &(const Rect &other) const;

	// Union

	template <typename X>
	friend Rect<T> Union(const Rect<T> &a, const Rect<T> &b, Hint hint);

	template <typename X>
	friend Rect<T> Union(const Rect<T> &a, const Rect<T> &b);

	Rect operator |(const Rect<T> &other) const;

	// Comparison

	bool operator ==(const Rect<T> &other) const;
	bool operator !=(const Rect<T> &other) const;

private:

	Vector2<T> m_mins;
	Vector2<T> m_maxs;
};

} // math


#include "../PopPack.h"
#include "Rect.inl"
