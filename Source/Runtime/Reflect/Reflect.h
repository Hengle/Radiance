// Reflect.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy & Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <boost/array.hpp>
#include "ReflectDef.h"
#include "../TypeTraits.h"
#include "../Container/ZoneVector.h"
#include "../PushPack.h"

namespace reflect {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::ITypeList
//////////////////////////////////////////////////////////////////////////////////////////

class ITypeList
{
public:

	virtual int Size() const = 0;
	virtual const Class *Type(int i) const = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IArgumentList
//////////////////////////////////////////////////////////////////////////////////////////

class IArgumentList : public ITypeList
{
public:

	virtual bool IsConst(int i) const = 0;
	virtual ConstReflected ConstArgument(int i) const = 0;
	virtual Reflected Argument(int i) const = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::MethodNotFoundException
//////////////////////////////////////////////////////////////////////////////////////////

RAD_DECLARE_EXCEPTION(MethodNotFoundException, exception);

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::InvalidCastException
//////////////////////////////////////////////////////////////////////////////////////////

RAD_DECLARE_EXCEPTION(InvalidCastException, exception);

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::ITyped
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS ITyped
{
public:

	// Get the type

	virtual const Class *Type() const = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IOwned
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS IOwned
{
public:

	// Get the owner type

	virtual const Class *OwnerType() const = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Attribute
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Attribute :
public PRIVATE_RADREFLECT_CLASS_BASE,
public ITyped
{
public:

	// Name access

	template <typename TChar>
	const TChar *Name() const;

	// Value access

	virtual const void *Value() const = 0;

protected:

	Attribute(const NAME &name);

private:

	NAME m_name;

	PRIVATE_RADREFLECT_FRIENDS
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::AttributeNotFoundException
//////////////////////////////////////////////////////////////////////////////////////////

RAD_DECLARE_EXCEPTION(AttributeNotFoundException, exception);

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Anonymous
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Anonymous
{
public:

	// Attribute access

	int NumAttributes() const;
	const ATTRIBUTE *Attribute(int i) const;

	const ATTRIBUTE *FindAttribute(const Class *type) const;
	template <typename TChar>
	const ATTRIBUTE *FindAttribute(const TChar *name) const;
	template<typename TChar>
	const ATTRIBUTE *FindAttribute(const Class *type, const TChar *name) const;

	const ATTRIBUTE *AttributeValue(const Class *type, ConstReflected &value) const;
	template<typename TChar>
	const ATTRIBUTE *AttributeValue(const TChar *name, ConstReflected &value) const;
	template<typename TChar>
	const ATTRIBUTE *AttributeValue(const Class *type, const TChar *name, ConstReflected &value) const;
	template<typename X>
	const ATTRIBUTE *AttributeValue(X &value) const;
	template<typename X, typename TChar>
	const ATTRIBUTE *AttributeValue(const TChar *name, X &value) const;

protected:

	Anonymous(
		const ATTRIBUTEARRAY &attributes
	);

private:

	const ATTRIBUTEARRAY m_attributes;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Named
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Named :
public PRIVATE_RADREFLECT_CLASS_BASE
{
public:

	// Attribute access

	int NumAttributes() const;
	const ATTRIBUTE *Attribute(int i) const;

	const ATTRIBUTE *FindAttribute(const Class *type) const;
	template <typename TChar>
	const ATTRIBUTE *FindAttribute(const TChar *name) const;
	template<typename TChar>
	const ATTRIBUTE *FindAttribute(const Class *type, const TChar *name) const;

	const ATTRIBUTE *AttributeValue(const Class *type, ConstReflected &value) const;
	template<typename TChar>
	const ATTRIBUTE *AttributeValue(const TChar *name, ConstReflected &value) const;
	template<typename TChar>
	const ATTRIBUTE *AttributeValue(const Class *type, const TChar *name, ConstReflected &value) const;
	template<typename X>
	const ATTRIBUTE *AttributeValue(X &value) const;
	template<typename X, typename TChar>
	const ATTRIBUTE *AttributeValue(const TChar *name, X &value) const;

	// Name access

	template <typename TChar>
	const TChar *Name() const;

protected:

	Named(
		const ATTRIBUTEARRAY &attributes,
		const NAME           &name
	);

private:

	const ATTRIBUTEARRAY m_attributes;
	const NAME           m_name;

};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Instanced
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Instanced :
public Named
{
public:

	// Get the offset from the start of the owner

	AddrSize Offset() const;

protected:

	Instanced(
		const ATTRIBUTEARRAY &attributes,
		const NAME           &name,
		AddrSize             offset
	);

	AddrSize m_offset;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::StaticVariable
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Static :
public Named
{
public:

	// Get the address

	void *Address() const;

protected:

	Static(
		const ATTRIBUTEARRAY &attributes,
		const NAME           &name,
		void                 *address
	);

	void *m_address;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::StaticConst
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS StaticConst :
public Named
{
public:

	// Get the address

	const void *Address() const;

protected:

	StaticConst(
		const ATTRIBUTEARRAY &attributes,
		const NAME           &name,
		const void           *address
	);

	const void *m_address;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IReturn
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS IReturn
{
public:

	// Get the return type

	virtual const Class *ReturnType() const = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS IFunction
{
public:

	// Forward declarations

	template <typename FunctionType> class CallException;
	template <typename FunctionType> class ArgumentException;
	template <typename FunctionType> class InvalidArgumentException;
	template <typename FunctionType> class MissingArgumentException;
	class Argument;

	// Type definitions

	typedef IFunction                          SelfType;
	typedef CallException<SelfType>            CallExceptionType;
	typedef ArgumentException<SelfType>        ArgumentExceptionType;
	typedef InvalidArgumentException<SelfType> InvalidArgumentExceptionType;
	typedef MissingArgumentException<SelfType> MissingArgumentExceptionType;

	typedef Argument                                        ARGUMENT;
	typedef ::reflect::details::ConstPointerArray<ARGUMENT> ARGUMENTARRAY;

	// Get arguments

	virtual int NumArguments() const = 0;
	virtual const ARGUMENT *Argument(int i) const = 0;

	// Throw exception types

	virtual void ThrowInvalidArgumentException(int argument) const = 0;
	virtual void ThrowMissingArgumentException(int argument) const = 0;

	// Exception descriptions

	virtual const char *InvalidArgumentDescription() const = 0;
	virtual const char *MissingArgumentDescription() const = 0;

	/////////////////////////////////////
	// reflect::IFunction::CallException
	/////////////////////////////////////

	template <typename FunctionType>
	class CallException :
	public CallException<typename FunctionType::SuperType>
	{
	public:

		typedef CallException<typename FunctionType::SuperType> SuperType;
		typedef CallException<FunctionType>                     SelfType;

		CallException(const FunctionType *function);
		virtual ~CallException () throw();

		const FunctionType *Function() const;

		SelfType &operator=(const SelfType &e);
	};

	/////////////////////////////////////
	// reflect::IFunction::IArgumentException
	/////////////////////////////////////

	class IArgumentException
	{
	public:

		virtual int ArgumentIndex() const = 0;
		virtual const ARGUMENT *Argument() const = 0;
	};

	/////////////////////////////////////
	// reflect::IFunction::ArgumentException<FunctionType>
	/////////////////////////////////////

	template <typename FunctionType>
	class ArgumentException :
	public FunctionType::CallExceptionType,
	public IArgumentException
	{
	public:

		typedef ArgumentException<FunctionType>          SelfType;
		typedef typename FunctionType::CallExceptionType SuperType;

		ArgumentException(const FunctionType *function, int argument);
		virtual ~ArgumentException() throw();

		int ArgumentIndex() const;
		const ARGUMENT *Argument() const;

		SelfType &operator=(const SelfType &e);

	private:

		int m_argument;

	};

	/////////////////////////////////////
	// reflect::IFunction::InvalidArgumentException<FunctionType>
	/////////////////////////////////////

	template <typename FunctionType>
	class InvalidArgumentException :
	public ArgumentException<FunctionType>
	{
	public:

		typedef InvalidArgumentException<FunctionType> SelfType;
		typedef ArgumentException<FunctionType>        SuperType;

		InvalidArgumentException(const FunctionType *function, int arg);
		virtual ~InvalidArgumentException() throw();

		virtual const char *what() const throw();

		SelfType &operator=(const SelfType &e);
	};

	/////////////////////////////////////
	// reflect::IFunction::MissingArgumentException<FunctionType>
	/////////////////////////////////////

	template <typename FunctionType>
	class MissingArgumentException :
	public ArgumentException<FunctionType>
	{
	public:

		typedef MissingArgumentException<FunctionType> SelfType;
		typedef ArgumentException<FunctionType>        SuperType;

		MissingArgumentException(const FunctionType *function, int arg);
		virtual ~MissingArgumentException() throw();

		virtual const char *what() const throw();

		SelfType &operator=(const SelfType &e);
	};

	/////////////////////////////////////
	// reflect::IFunction::Argument
	/////////////////////////////////////

	class RADRT_CLASS Argument :
	public Named,
	public ITyped
	{
	public:

		// Is it a const argument?

		virtual bool IsConst() const = 0;

		// Is it a reference argument?

		virtual bool IsReference() const = 0;

		// Of type Type()

		virtual const void *DefaultValue() const = 0;

	protected:

		Argument(
			const ATTRIBUTEARRAY &attributes,
			const NAME           &name
		);

		PRIVATE_RADREFLECT_FRIENDS
	};
};

template <>
class IFunction::CallException<IFunction> :
public exception
{
public:

	typedef CallException<IFunction> SelfType;

	CallException(const IFunction *function);
	virtual ~CallException() throw();

	const IFunction *Function() const;

	SelfType &operator=(const SelfType &e);

protected:

	const IFunction *m_function;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::NamedFunction
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS NamedFunction :
public Named,
public IFunction
{
public:

	typedef IFunction                          SuperType;
	typedef NamedFunction                      SelfType;
	typedef CallException<SelfType>            CallExceptionType;
	typedef ArgumentException<SelfType>        ArgumentExceptionType;
	typedef InvalidArgumentException<SelfType> InvalidArgumentExceptionType;
	typedef MissingArgumentException<SelfType> MissingArgumentExceptionType;

	// IFunction implementation

	virtual const char *InvalidArgumentDescription() const;
	virtual const char *MissingArgumentDescription() const;

	// Argument access

	virtual int NumArguments() const;
	virtual const ARGUMENT *Argument(int i) const;

protected:

	NamedFunction(
		const ATTRIBUTEARRAY &attributes,
		const NAME           &name,
		const ARGUMENTARRAY  &arguments
	);

private:

	const ARGUMENTARRAY m_arguments;

	PRIVATE_RADREFLECT_FRIENDS
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::AnonymousFunction
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS AnonymousFunction :
public Anonymous,
public IFunction
{
public:

	typedef IFunction                          SuperType;
	typedef AnonymousFunction                  SelfType;
	typedef CallException<SelfType>            CallExceptionType;
	typedef ArgumentException<SelfType>        ArgumentExceptionType;
	typedef InvalidArgumentException<SelfType> InvalidArgumentExceptionType;
	typedef MissingArgumentException<SelfType> MissingArgumentExceptionType;

	// IFunction implementation

	virtual const char *InvalidArgumentDescription() const;
	virtual const char *MissingArgumentDescription() const;

	// Argument access

	int NumArguments() const;
	const ARGUMENT *Argument(int i) const;

protected:

	AnonymousFunction(
		const ATTRIBUTEARRAY &attributes,
		const ARGUMENTARRAY  &arguments
	);

private:

	const ARGUMENTARRAY m_arguments;

	PRIVATE_RADREFLECT_FRIENDS
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Function
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Function :
public NamedFunction,
public IReturn
{
public:

	typedef NamedFunction                      SuperType;
	typedef Function                           SelfType;
	typedef CallException<SelfType>            CallExceptionType;
	typedef ArgumentException<SelfType>        ArgumentExceptionType;
	typedef InvalidArgumentException<SelfType> InvalidArgumentExceptionType;
	typedef MissingArgumentException<SelfType> MissingArgumentExceptionType;

	// Call the function

	virtual void Call(
		const Reflected &result,
		const IArgumentList &args
	) const = 0; // throw (CallExceptionType)

	// Throw exception types

	virtual void ThrowInvalidArgumentException(int argument) const;
	virtual void ThrowMissingArgumentException(int argument) const;

protected:

	Function(
		const ATTRIBUTEARRAY &attributes,
		const NAME           &name,
		const ARGUMENTARRAY  &arguments
	);

private:

	PRIVATE_RADREFLECT_FRIENDS
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class
//////////////////////////////////////////////////////////////////////////////////////////

RAD_CLR_PUBLIC class RADRT_CLASS Class :
public Named
{
public:

	// Forward declarations

	class Super;

	class Member;
	class StaticMember;
	class StaticConstant;

	class Constructor;
	class Method;
	class ConstMethod;
	class MutableMethod;
	class StaticMethod;

	// Type definitions

	typedef Super          SUPER;
	typedef Member         MEMBER;
	typedef StaticMember   STATICMEMBER;
	typedef StaticConstant STATICCONSTANT;
	typedef Constructor    CONSTRUCTOR;
	typedef Method         METHOD;

	// Method subclass type definitions

	typedef ConstMethod    CONSTMETHOD;
	typedef MutableMethod  MUTABLEMETHOD;
	typedef StaticMethod   STATICMETHOD;

	// Array type definitions

	typedef ::reflect::details::ConstPointerArray<SUPER>          SUPERARRAY;
	typedef ::reflect::details::ConstPointerArray<MEMBER>         MEMBERARRAY;
	typedef ::reflect::details::ConstPointerArray<STATICMEMBER>   STATICMEMBERARRAY;
	typedef ::reflect::details::ConstPointerArray<STATICCONSTANT> STATICCONSTANTARRAY;
	typedef ::reflect::details::ConstPointerArray<CONSTRUCTOR>    CONSTRUCTORARRAY;
	typedef ::reflect::details::ConstPointerArray<CONSTMETHOD>    CONSTMETHODARRAY;
	typedef ::reflect::details::ConstPointerArray<MUTABLEMETHOD>  MUTABLEMETHODARRAY;
	typedef ::reflect::details::ConstPointerArray<STATICMETHOD>   STATICMETHODARRAY;

	// Destructor

	~Class();

	// Access

	AddrSize Alignment() const;
	AddrSize Size() const;

	// Methods

	bool IsA(const Class *type) const;
	virtual const Class *ConstType() const = 0;

	// Lookup a class by name

	template <typename TChar>
	static const Class *Find(const TChar *name);

	// Destroy

	void Destroy(void *object) const;

	/////////////////////////////////////
	// reflect::Class::Super
	/////////////////////////////////////

	class RADRT_CLASS Super :
	public Instanced,
	public ITyped,
	public IOwned
	{
	protected:

		Super(
			const ATTRIBUTEARRAY &attributes,
			const NAME           &name,
			AddrSize             offset
		);

		PRIVATE_RADREFLECT_FRIENDS
	};

	// Super access

	int NumSupers() const;
	const SUPER *Super(int i) const;

	// Find Super

	template <typename TChar>
	const SUPER *FindSuper(const TChar *name, bool recurse = false) const;

	/////////////////////////////////////
	// reflect::Class::Member
	/////////////////////////////////////

	class RADRT_CLASS Member :
	public Instanced,
	public ITyped,
	public IOwned
	{
	protected:

		Member(
			const ATTRIBUTEARRAY &attributes,
			const NAME           &name,
			AddrSize             offset
		);

		PRIVATE_RADREFLECT_FRIENDS
	};

	// Member access

	int NumMembers() const;
	const MEMBER *Member(int i) const;

	template <typename TChar>
	const MEMBER *FindMember(const TChar *name, bool findInSupers = false) const;

	/////////////////////////////////////
	// reflect::Class::StaticMember
	/////////////////////////////////////

	class RADRT_CLASS StaticMember :
	public Static,
	public ITyped,
	public IOwned
	{
	protected:

		StaticMember(
			const ATTRIBUTEARRAY &attributes,
			const NAME           &name,
			void                 *address
		);

		PRIVATE_RADREFLECT_FRIENDS
	};

	// Static member access

	int NumStaticMembers() const;
	const STATICMEMBER *StaticMember(int i) const;

	template <typename TChar>
	const STATICMEMBER *FindStaticMember(const TChar *name, bool findInSupers = false) const;

	/////////////////////////////////////
	// reflect::Class::StaticConstant
	/////////////////////////////////////

	class RADRT_CLASS StaticConstant :
	public StaticConst,
	public ITyped,
	public IOwned
	{
	protected:

		StaticConstant(
			const ATTRIBUTEARRAY &attributes,
			const NAME           &name,
			const void           *address
		);

		PRIVATE_RADREFLECT_FRIENDS
	};

	// Static constant access

	int NumStaticConstants() const;
	const STATICCONSTANT *StaticConstant(int i) const;

	template <typename TChar>
	const STATICCONSTANT *FindStaticConstant(const TChar *name, bool findInSupers = false) const;

	/////////////////////////////////////
	// reflect::Class::Constructor
	/////////////////////////////////////

	class RADRT_CLASS Constructor :
	public AnonymousFunction,
	public IOwned
	{
	public:

		typedef AnonymousFunction                  SuperType;
		typedef CONSTRUCTOR                        SelfType;
		typedef CallException<SelfType>            CallExceptionType;
		typedef ArgumentException<SelfType>        ArgumentExceptionType;
		typedef InvalidArgumentException<SelfType> InvalidArgumentExceptionType;
		typedef MissingArgumentException<SelfType> MissingArgumentExceptionType;

		// Match an argument list

		virtual bool Match(const ITypeList &args) const = 0;

		// Call the constructor

		virtual void Call(
			void          *place,
			const IArgumentList &args
		) const = 0; // throw (CallExceptionType)

		// Throw exception types

		virtual void ThrowInvalidArgumentException(int argument) const;
		virtual void ThrowMissingArgumentException(int argument) const;

	protected:

		Constructor(
			const ATTRIBUTEARRAY &attributes,
			const ARGUMENTARRAY  &arguments
		);

	private:

		PRIVATE_RADREFLECT_FRIENDS
	};

	// Constructor access

	int NumConstructors() const;
	const CONSTRUCTOR *Constructor(int i) const;

	// Find Constructor

	template <typename FunctionType>
	const CONSTRUCTOR *FindConstructor() const;

	const CONSTRUCTOR *FindConstructor(const ITypeList &args) const;

	// Find and call Constructor

	template <typename FunctionType>
	const CONSTRUCTOR *Construct(void *place, const IArgumentList &args) const;

	const CONSTRUCTOR *Construct(void *place, const IArgumentList &args) const;

	/////////////////////////////////////
	// reflect::Class::Method
	/////////////////////////////////////

	class RADRT_CLASS Method :
	public NamedFunction,
	public IOwned,
	public IReturn
	{
	public:

		typedef NamedFunction                      SuperType;
		typedef METHOD                             SelfType;
		typedef CallException<SelfType>            CallExceptionType;
		typedef ArgumentException<SelfType>        ArgumentExceptionType;
		typedef InvalidArgumentException<SelfType> InvalidArgumentExceptionType;
		typedef MissingArgumentException<SelfType> MissingArgumentExceptionType;

		// Match an argument list

		virtual bool Match(const ITypeList &args) const = 0;

	protected:

		Method(
			const ATTRIBUTEARRAY &attributes,
			const NAME           &name,
			const ARGUMENTARRAY  &arguments
		);

	private:

		PRIVATE_RADREFLECT_FRIENDS
	};

	/////////////////////////////////////
	// reflect::Class::ConstMethod
	/////////////////////////////////////

	class RADRT_CLASS ConstMethod :
	public METHOD
	{
	public:

		typedef METHOD                             SuperType;
		typedef CONSTMETHOD                        SelfType;
		typedef CallException<SelfType>            CallExceptionType;
		typedef ArgumentException<SelfType>        ArgumentExceptionType;
		typedef InvalidArgumentException<SelfType> InvalidArgumentExceptionType;
		typedef MissingArgumentException<SelfType> MissingArgumentExceptionType;

		// Call the method

		virtual void Call(
			const Reflected     &result,
			const void          *object,
			const IArgumentList &args
		) const = 0; // throw (CallExceptionType)

		// Throw exception types

		virtual void ThrowInvalidArgumentException(int argument) const;
		virtual void ThrowMissingArgumentException(int argument) const;

	protected:

		ConstMethod(
			const ATTRIBUTEARRAY &attributes,
			const NAME           &name,
			const ARGUMENTARRAY  &arguments
		);

		PRIVATE_RADREFLECT_FRIENDS
	};

	// ConstMethod access

	int NumConstMethods() const;
	const CONSTMETHOD *ConstMethod(int i) const;

	// Find ConstMethod

	template <typename FunctionType, typename TChar>
	const CONSTMETHOD *FindConstMethod(
		const TChar *name,
		bool findInSupers = false
	) const;

	template <typename TChar>
	const CONSTMETHOD *FindConstMethod(
		const TChar *name,
		const ITypeList &list,
		bool findInSupers = false
	) const;

	/////////////////////////////////////
	// reflect::Class::MutableMethod
	/////////////////////////////////////

	class RADRT_CLASS MutableMethod :
	public METHOD
	{
	public:

		typedef METHOD                             SuperType;
		typedef MUTABLEMETHOD                      SelfType;
		typedef CallException<SelfType>            CallExceptionType;
		typedef ArgumentException<SelfType>        ArgumentExceptionType;
		typedef InvalidArgumentException<SelfType> InvalidArgumentExceptionType;
		typedef MissingArgumentException<SelfType> MissingArgumentExceptionType;

		// Call the method

		virtual void Call(
			const Reflected     &result,
			void                *object,
			const IArgumentList &args
		) const = 0; // throw (CallExceptionType)

		// Throw exception types

		virtual void ThrowInvalidArgumentException(int argument) const;
		virtual void ThrowMissingArgumentException(int argument) const;

	protected:

		MutableMethod(
			const ATTRIBUTEARRAY &attributes,
			const NAME           &name,
			const ARGUMENTARRAY  &arguments
		);

		PRIVATE_RADREFLECT_FRIENDS
	};

	// Method access

	int NumMethods() const;
	const MUTABLEMETHOD *Method(int i) const;

	// Find Method

	template <typename FunctionType, typename TChar>
	const MUTABLEMETHOD *FindMethod(
		const TChar *name,
		bool findInSupers = false
	) const;

	template <typename TChar>
	const MUTABLEMETHOD *FindMethod(
		const TChar *name,
		const ITypeList &list,
		bool findInSupers = false
	) const;

	/////////////////////////////////////
	// reflect::Class::StaticMethod
	/////////////////////////////////////

	class RADRT_CLASS StaticMethod :
	public METHOD
	{
	public:

		typedef METHOD                             SuperType;
		typedef STATICMETHOD                       SelfType;
		typedef CallException<SelfType>            CallExceptionType;
		typedef ArgumentException<SelfType>        ArgumentExceptionType;
		typedef InvalidArgumentException<SelfType> InvalidArgumentExceptionType;
		typedef MissingArgumentException<SelfType> MissingArgumentExceptionType;

		// Call the method

		virtual void Call(
			const Reflected     &result,
			const IArgumentList &args
		) const = 0; // throw (CallExceptionType)

		// Throw exception types

		virtual void ThrowInvalidArgumentException(int argument) const;
		virtual void ThrowMissingArgumentException(int argument) const;

	protected:

		StaticMethod(
			const ATTRIBUTEARRAY &attributes,
			const NAME           &name,
			const ARGUMENTARRAY  &arguments
		);

		PRIVATE_RADREFLECT_FRIENDS
	};

	// StaticMethod access

	int NumStaticMethods() const;
	const STATICMETHOD *StaticMethod(int i) const;

	// Find StaticMethod

	template <typename FunctionType, typename TChar>
	const STATICMETHOD *FindStaticMethod(
		const TChar *name,
		bool findInSupers = false
	) const;

	template <typename TChar>
	const STATICMETHOD *FindStaticMethod(
		const TChar *name,
		const ITypeList &list, bool
		findInSupers = false
	) const;

	// Find and call StaticMethod

	template <typename FunctionType, typename TChar>
	const STATICMETHOD *FindAndCallStaticMethod(
		const Reflected     &result,
		const TChar         *name,
		const IArgumentList &args,
		bool findInSupers = false
	) const;

	template <typename TChar>
	const STATICMETHOD *FindAndCallStaticMethod(
		const Reflected     &result,
		const TChar         *name,
		const IArgumentList &args,
		bool findInSupers = false
	) const;

protected:

	typedef void (*DTOR)(const Class *, void *);

	Class(
		const ATTRIBUTEARRAY      &attributes,
		const NAME                &name,
		AddrSize                  size,
		AddrSize                  alignment,
		const SUPERARRAY          &supers,
		const MEMBERARRAY         &members,
		const STATICMEMBERARRAY   &staticMembers,
		const STATICCONSTANTARRAY &staticConstants,
		const CONSTRUCTORARRAY    &constructors,
		const MUTABLEMETHODARRAY  &methods,
		const CONSTMETHODARRAY    &constMethods,
		const STATICMETHODARRAY   &staticMethods,
		DTOR                      dtor
	);

private:

	// Data

	const SUPERARRAY          m_supers;
	const MEMBERARRAY         m_members;
	const STATICMEMBERARRAY   m_staticMembers;
	const STATICCONSTANTARRAY m_staticConstants;
	const CONSTRUCTORARRAY    m_constructors;
	const MUTABLEMETHODARRAY  m_methods;
	const CONSTMETHODARRAY    m_constMethods;
	const STATICMETHODARRAY   m_staticMethods;

	AddrSize                  m_size;

	// Callbacks

	DTOR                      m_dtor;

	U8                        m_alignment;

	PRIVATE_RADREFLECT_FRIENDS
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Type<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
const Class *Type();

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::TypeEnumerator
//////////////////////////////////////////////////////////////////////////////////////////

class TypeEnumerator
{
public:

	TypeEnumerator();
	TypeEnumerator(const Class *pos);
	~TypeEnumerator();

	void Begin(const Class *pos);
	const Class *Begin();
	const Class *Next();
	const Class *Prev();
	const Class *End();
	void Release();

	// After Begin()/End() is called, the TypeEnumerator will have locked the class list,
	// meaning all DLL loads / unloads that register or tear down types will block
	// until Release() is called, or the TypeEnumerator goes out of scope.

private:

	TypeEnumerator(const TypeEnumerator&);
	TypeEnumerator &operator = (const TypeEnumerator&);
	const Class *m_pos;
	bool m_locked;
};

} // reflect


#include "../PopPack.h"

#include "Attributes.h"
#include "RTLInterop.h"
#include <vector>
#include "../PushPack.h"


namespace reflect {

template <typename T, bool IsConst = meta::IsConst<T>::VALUE>
struct ReflectResult;

template <typename T, bool IsConst>
struct ReflectResult
{
	typedef Reflected Type;
	typedef void      VoidType;
};

template <typename T>
struct ReflectResult<T, true>
{
	typedef ConstReflected Type;
	typedef const void     VoidType;
};

template <>
struct ReflectResult<const ATTRIBUTE*>
{
	typedef ConstReflected Type;
	typedef void           VoidType;
};

template <>
struct ReflectResult<const Class::STATICMEMBER*>
{
	typedef Reflected Type;
	typedef void      VoidType;
};

template <>
struct ReflectResult<const Class::STATICCONSTANT*>
{
	typedef ConstReflected Type;
	typedef void           VoidType;
};

template <>
struct ReflectResult<const IFunction::ARGUMENT*>
{
	typedef ConstReflected Type;
	typedef void           VoidType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflect<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
typename ReflectResult<T>::Type Reflect(T &obj);

Reflected Reflect(void *data, const Class *type);
ConstReflected Reflect(const void *data, const Class *type);

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected
//////////////////////////////////////////////////////////////////////////////////////////
//
// A ConstReflected type cannot be modified, and is therefore a const variable.
//
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS ConstReflected
{
public:

	// Constructors

	ConstReflected();
	ConstReflected(const ConstReflected& reflected);

	// Destructor

	~ConstReflected();

	// Template conversion operator (does a class cast).

	template <typename T>
	operator const T *() const;

	const void *Data() const;

	// Access

	const Class *Type() const;
	void Close();
	bool IsValid() const;

	// Attribute access

	int NumAttributes() const;
	ConstReflected Attribute(int i) const;

	// Super access

	int NumSupers() const;
	ConstReflected Super(int i) const;
	ConstReflected Super(const Class::SUPER *super) const;

	// Throws InvalidCastException if a match is not found.

	template <typename TChar>
	ConstReflected Cast(const TChar *typeName) const;

	ConstReflected Cast(const Class *type) const;

	// Unsafe cast, does not throw an exception (unless the type is not found), just blindly changes the class.

	template <typename TChar>
	ConstReflected UnsafeCast(const TChar *typeName) const;
	ConstReflected UnsafeCast(const Class *type) const;

	// Member access

	int NumMembers() const;
	ConstReflected Member(int i) const;
	ConstReflected Member(const Class::MEMBER *member, bool cast = true) const;

	// Find - these will recurse base classes
	// in a depth-first search until a match is found

	template <typename TChar>
	ConstReflected FindSuper(const TChar *name, bool recurse = false) const;

	template <typename TChar>
	ConstReflected FindMember(const TChar *name, bool findInSupers = false) const;

	template <typename FunctionType, typename TChar>
	const Class::CONSTMETHOD *FindConstMethod(const TChar *name, bool findInSupers = false) const;

	template <typename TChar>
	const Class::CONSTMETHOD *FindConstMethod(
		const TChar *name,
		const ITypeList &args,
		bool  findInSupers = false
	) const;

	template <typename FunctionType, typename TChar>
	const Class::CONSTMETHOD *FindAndCallConstMethod(
		const Reflected &result,
		const TChar   *name,
		const IArgumentList &args,
		bool findInSupers = false
	) const;

	template <typename TChar>
	const Class::CONSTMETHOD *FindAndCallConstMethod(
		const Reflected &result,
		const TChar   *name,
		const IArgumentList &args,
		bool findInSupers = false
	) const;

	void CallConstMethod(
		const Reflected &result,
		const Class::CONSTMETHOD *method,
		const IArgumentList &args
	) const;

	// Assignment

	ConstReflected &operator=(const ConstReflected &reflected);

	// Bool

	operator bool () const;

protected:

	ConstReflected(const void *data, const Class *type);

	template <typename TChar>
	ConstReflected CastByName(const TChar *typeName) const;
	ConstReflected CastByType(const Class *type) const;

	template <typename T>
	ConstReflected UncheckedCastByType(const T *type) const;

	friend class IArgumentList;
	friend ConstReflected Reflect(const void *data, const Class *type);

	const U8    *m_data;
	const Class *m_type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected
//////////////////////////////////////////////////////////////////////////////////////////

RAD_CLR_PUBLIC class RADRT_CLASS Reflected :
public ConstReflected
{

public:

	// Constructors

	Reflected();
	Reflected(const Reflected &reflected);

	// Destructor

	~Reflected();

	// Template conversion operator (does a class cast)

	template <typename T>
	operator T *() const;

	void *Data() const;

	// Throws InvalidCastException if a match is not found.

	template <typename TChar>
	Reflected Cast(const TChar *typeName) const;
	Reflected Cast(const Class *type) const;

	// Unsafe cast, does not throw an exception (unless the type is not found), just blindly changes the class.

	template <typename TChar>
	Reflected UnsafeCast(const TChar *typeName) const;
	Reflected UnsafeCast(const Class *type) const;

	// Super access

	int NumSupers() const;
	Reflected Super(int i) const;
	Reflected Super(const Class::SUPER *type) const;

	// Member access

	int NumMembers() const;
	Reflected Member(int i) const;
	Reflected Member(const Class::MEMBER *member, bool cast = true) const;

	// Find - these will recurse base classes
	// in a depth-first search until a match is found

	template <typename TChar>
	Reflected FindSuper(const TChar *name) const;

	template <typename TChar>
	Reflected FindMember(const TChar *name) const;

	template <typename FunctionType, typename TChar>
	const Class::MUTABLEMETHOD *FindMethod(
			const TChar *name,
			bool findInSupers = false
	) const;

	template <typename TChar>
	const Class::MUTABLEMETHOD *FindMethod(
		const TChar *name,
		const ITypeList &list,
		bool findInSupers = false
	) const;

	template <typename FunctionType, typename TChar>
	const Class::MUTABLEMETHOD *FindAndCallMethod(
		const Reflected  &result,
		const TChar   *name,
		const IArgumentList &args,
		bool findInSupers = false
	) const;

	template <typename TChar>
	const Class::MUTABLEMETHOD *FindAndCallMethod(
		const Reflected  &result,
		const TChar   *name,
		const IArgumentList &args,
		bool findInSupers = false
	) const;

	void CallMethod(
		const Reflected &result,
		const Class::MUTABLEMETHOD *method,
		const IArgumentList &args
	) const;

	// Assignment

	Reflected &operator=(const Reflected &reflected);

	// New and delete
	// Note: New/Clone must be Delete()'ed. Make sure
	// you call this or wrap it in a SharedReflected, or ReflectedRef

	template <typename FunctionType>
	static Reflected New(
		Zone                   &zone,
		const Class            *type,
		const IArgumentList    &args,
		void                   *data = 0, // null means allocate data for me.
		UReg                    alignment = 0 // zero means use Class->Alignment()
	);

	static Reflected New(
		Zone                   &zone,
		const Class            *type,
		const IArgumentList    &args,
		void                   *data = 0,
		UReg                    alignment = 0
	);

	static Reflected New(
		Zone                     &zone,
		const Class              *type,
		const Class::CONSTRUCTOR *constructor,
		const IArgumentList      &args,
		void                     *data = 0,
		UReg                      alignment = 0
	);

	// Clone via copy construction.
	// Type must have a copy constructor or be a basic type.
	static Reflected Clone(
		Zone &zone,
		const ConstReflected &r
	);

	void Delete();
	void Destroy();

protected:

	explicit Reflected(const ConstReflected& reflected);

private:

	Reflected(void *data, const Class *type);

	friend class Array;
	friend class IArgumentList;
	template <typename T> friend typename ReflectResult<T>::Type Reflect(T &obj);
	friend Reflected Reflect(void *data, const Class *type);
};

//
// A reflected object that is destructed and then free'd when it goes out of scope. This object must have been
// created using Reflected::New()
//

class SharedReflected : public Reflected
{
public:
	typedef boost::shared_ptr<SharedReflected> Ref;
	SharedReflected() {}
	SharedReflected(const Reflected &r) : Reflected(r) {}
	~SharedReflected() { Delete(); }

	SharedReflected &operator = (const Reflected &r) { Reflected::operator = (r); return *this; }
};

//
// A reflected object that is destructed (but not free'd) when it goes out of scope.
//

class ReflectedRef : public Reflected
{
public:

	ReflectedRef() {}
	ReflectedRef(const Reflected &r) : Reflected(r) {}
	~ReflectedRef() { Destroy(); }

	ReflectedRef &operator = (const Reflected &r) { Reflected::operator = (r); return *this; }
};

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::BaseArgumentList
//////////////////////////////////////////////////////////////////////////////////////////

class BaseArgumentList :
public IArgumentList
{
protected:

	struct ARG :
	public Reflected
	{
		ARG() : m_isConst(false) {}
		ARG(bool isConst, const ConstReflected &r) : Reflected(r), m_isConst(isConst) {}
		ARG(const ARG &a) : Reflected(a), m_isConst(a.m_isConst) {}

		~ARG() {}

		ARG &operator=(const ARG &o)
		{
			m_isConst = o.m_isConst;
			Reflected::operator=(o);
			return *this;
		}

		bool m_isConst;
	};

};

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ArgumentList
//////////////////////////////////////////////////////////////////////////////////////////

class ArgumentList :
public BaseArgumentList
{
public:

	ArgumentList();
	~ArgumentList();

	// Add arguments

	ArgumentList &PushBack(const ConstReflected &r);
	ArgumentList &PushBack(const Reflected &r);

	// IArgumentList

	virtual int Size() const;
	virtual const Class *Type(int i) const;
	virtual bool IsConst(int i) const;
	virtual ConstReflected ConstArgument(int i) const;
	virtual Reflected Argument(int i) const;

private:

	typedef zone_vector<ARG, ZRuntimeT>::type LIST;
	LIST m_list;
};

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedArgumentList
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
class FixedArgumentList :
public BaseArgumentList
{
public:

	FixedArgumentList();
	~FixedArgumentList();

	// Add arguments

	FixedArgumentList<NUMARGS> &PushBack(const ConstReflected &r);
	FixedArgumentList<NUMARGS> &PushBack(const Reflected &r);

	// IArgumentList

	virtual int Size() const;
	virtual const Class *Type(int i) const;
	virtual bool IsConst(int i) const;
	virtual ConstReflected ConstArgument(int i) const;
	virtual Reflected Argument(int i) const;

private:

	typedef boost::array<ARG, NUMARGS> LIST;

	LIST m_list;
	AddrSize m_ofs;
};

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::NullArgs
//////////////////////////////////////////////////////////////////////////////////////////

class NullArgs : public IArgumentList
{
public:

	static IArgumentList &Get()
	{
		static NullArgs x;
		return x;
	}

	virtual int Size() const;
	virtual const Class *Type(int i) const;
	virtual bool IsConst(int i) const;
	virtual ConstReflected ConstArgument(int i) const;
	virtual Reflected Argument(int i) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::TypeList
//////////////////////////////////////////////////////////////////////////////////////////

class TypeList :
public ITypeList
{
public:

	TypeList();
	~TypeList();

	// Add arguments

	TypeList &PushBack(const Class *type);

	// ITypeList

	virtual int Size() const;
	virtual const Class *Type(int i) const;

private:

	typedef zone_vector<const Class *, ZRuntimeT>::type LIST;
	LIST m_list;
};

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedTypeList
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
class FixedTypeList :
public ITypeList
{
public:

	FixedTypeList();
	~FixedTypeList();

	// Add arguments

	FixedTypeList<NUMARGS> &PushBack(const Class *type);

	// ITypeList

	virtual int Size() const;
	virtual const Class *Type(int i) const;

private:

	typedef boost::array<const Class*, NUMARGS> LIST;
	LIST m_list;
	AddrSize m_ofs;
};

void *Allocate(Zone &zone, const Class *type, AddrSize alignment=0);
void *SafeAllocate(Zone &zone, const Class *type, AddrSize alignment=0);
void Free(void *data);

} // reflect


//////////////////////////////////////////////////////////////////////////////////////////
// RAD_REFLECTED_STACK_VAR_CONSTRUCTOR_ARGS_ALIGN
//////////////////////////////////////////////////////////////////////////////////////////

#define RAD_REFLECTED_STACK_VAR_CONSTRUCTOR_ARGS_ALIGN(_name_, _classptr_, _constructor_, _args_, _alignment_) \
	RAD_ASSERT(_classptr_);\
	RAD_ASSERT_MSG(_constructor_, "Invalid Constructor!");\
	::reflect::Reflected _name_;\
	{\
		AddrSize align = _alignment_;\
		if (!align) { align = (_classptr_)->Alignment(); }\
		RAD_ASSERT(::IsPowerOf2(align));\
		void *data = ::Align(stack_alloc((_classptr_)->Size() + (align) - 1), align);\
		_name_ = ::reflect::Reflect(data, _classptr_);\
		(_constructor_)->Call(data, _args_);\
	}\
	::reflect::ReflectedRef RAD_JOIN(__rad_auto_destroy_reflected_, _name_)(_name_)

//////////////////////////////////////////////////////////////////////////////////////////
// RAD_REFLECTED_STACK_VAR_CONSTRUCTOR_ARGS
//////////////////////////////////////////////////////////////////////////////////////////

#define RAD_REFLECTED_STACK_VAR_CONSTRUCTOR_ARGS(_name_, _classptr_, _constructor_, _args_)\
	RAD_REFLECTED_STACK_VAR_CONSTRUCTOR_ARGS_ALIGN(_name_, _classptr_, _constructor_, _args_, 0)

//////////////////////////////////////////////////////////////////////////////////////////
// RAD_REFLECTED_STACK_VAR_ARGS_ALIGN
//////////////////////////////////////////////////////////////////////////////////////////

#define RAD_REFLECTED_STACK_VAR_ARGS_ALIGN(_name_, _classptr_, _fn_type_, _args_, _alignment_)\
	RAD_REFLECTED_STACK_VAR_CONSTRUCTOR_ARGS_ALIGN(_name_, _classptr_, \
		_classptr_->FindConstructor<_fn_type_>(), _args_, _alignment_)

//////////////////////////////////////////////////////////////////////////////////////////
// RAD_REFLECTED_STACK_VAR_ALIGN
//////////////////////////////////////////////////////////////////////////////////////////

#define RAD_REFLECTED_STACK_VAR_ALIGN(_name_, _classptr_, _alignment_)\
	RAD_REFLECTED_STACK_VAR_ARGS_ALIGN(_name_, _classptr_, void (), ::reflect::NullArgs(), _alignment_)

//////////////////////////////////////////////////////////////////////////////////////////
// RAD_REFLECTED_STACK_VAR
//////////////////////////////////////////////////////////////////////////////////////////

#define RAD_REFLECTED_STACK_VAR_ARGS(_name_, _classptr_, _fn_type_, _args_)\
	RAD_REFLECTED_STACK_VAR_ARGS_ALIGN(_name_, _classptr_, _fn_type_, _args_, 0)

//////////////////////////////////////////////////////////////////////////////////////////
// RAD_REFLECTED_STACK_VAR
//////////////////////////////////////////////////////////////////////////////////////////

#define RAD_REFLECTED_STACK_VAR(_name_, _classptr_)\
	RAD_REFLECTED_STACK_VAR_ALIGN(_name_, _classptr_, 0)

#include "../PopPack.h"
#include "Private/ReflectPrivate.h"
#include "Reflect.inl"

//////////////////////////////////////////////////////////////////////////////////////////
// Reflection Declarations
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_DECLARE(RADRT_API, ::reflect::Reflected)
RADREFLECT_DECLARE(RADRT_API, ::reflect::ConstReflected)
RADREFLECT_DECLARE(RADRT_API, ::reflect::Class*)
RADREFLECT_DECLARE(RADRT_API, const ::reflect::Class*)
RADREFLECT_DECLARE(RADRT_API, ::reflect::Class::MEMBER*)
RADREFLECT_DECLARE(RADRT_API, const ::reflect::Class::MEMBER*)
RADREFLECT_DECLARE(RADRT_API, ::reflect::Class::STATICMEMBER*)
RADREFLECT_DECLARE(RADRT_API, const ::reflect::Class::STATICMEMBER*)
RADREFLECT_DECLARE(RADRT_API, ::reflect::Class::STATICCONSTANT*)
RADREFLECT_DECLARE(RADRT_API, const ::reflect::Class::STATICCONSTANT*)
RADREFLECT_DECLARE(RADRT_API, ::reflect::Class::MUTABLEMETHOD*)
RADREFLECT_DECLARE(RADRT_API, const ::reflect::Class::MUTABLEMETHOD*)
RADREFLECT_DECLARE(RADRT_API, ::reflect::Class::CONSTMETHOD*)
RADREFLECT_DECLARE(RADRT_API, const ::reflect::Class::CONSTMETHOD*)
RADREFLECT_DECLARE(RADRT_API, ::reflect::Class::STATICMETHOD*)
RADREFLECT_DECLARE(RADRT_API, const ::reflect::Class::STATICMETHOD*)
