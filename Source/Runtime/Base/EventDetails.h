// EventDetails.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

class BaseEvent;
namespace details {

template <typename T>
class EventHandler
{
public:
	typedef EventHandler<T> TSelf;
	typedef boost::shared_ptr<TSelf> Ref;

	virtual void Invoke(const T &t) = 0;
	virtual bool Equals(void *a, void *b) = 0;
	virtual void DestructNotify(BaseEvent *e) = 0;
};

template <>
class EventHandler<void>
{
public:
	typedef EventHandler<void> TSelf;
	typedef boost::shared_ptr<TSelf> Ref;

	virtual void Invoke() = 0;
	virtual bool Equals(void *a, void *b) = 0;
	virtual void DestructNotify(BaseEvent *e) = 0;
};

} // details
