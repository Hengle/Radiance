// ReflectTest.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#include "../UTCommon.h"
#include "ReflectTest.h"
#include <Runtime/Reflect.h>
#include <iostream>


using ut::filllevel;

//////////////////////////////////////////////////////////////////////////////////////////
// ReflectClassTest statics
//////////////////////////////////////////////////////////////////////////////////////////

float ReflectClassTest::s_x = 0;
const float ReflectClassTest::CONST_Y = 23.0f;

//////////////////////////////////////////////////////////////////////////////////////////
// DumpClassInfo()
//////////////////////////////////////////////////////////////////////////////////////////

static void DumpClassInfo(const reflect::ConstReflected& ref, int level = 0)
{
	//const reflect::Class* b = reflect::Type<bool>();
	const reflect::Class* c = ref.Type();
	RAD_ASSERT(NULL != c);

	// Output class attributes

	std::cout << std::endl << filllevel(level);
	std::cout << "Type " << ref.Type()->Name<char>() << " has " << ref.NumAttributes() << " attributes." << std::endl;
	for (int i = 0; i < ref.NumAttributes(); i++)
	{
		std::cout << filllevel(level + 1);
		std::cout << ref.Type()->Attribute(i)->Type()->Name<char>();
		std::cout << " " << ref.Type()->Attribute(i)->Name<char>();
		std::cout << std::endl;
	}

	// Output supertypes

	std::cout << std::endl << filllevel(level);
	std::cout << "Type " << ref.Type()->Name<char>() << " has " << ref.NumSupers() << " supers." << std::endl;
	for (int i = 0; i < ref.NumSupers(); i++)
	{
		reflect::ConstReflected superRef = ref.Super(i);
		DumpClassInfo(superRef, level + 1);
	}

	// Output members

	std::cout << std::endl << filllevel(level);
	std::cout << "Type " << ref.Type()->Name<char>() << " has " << ref.NumMembers() << " members." << std::endl;
	for (int i = 0; i < ref.NumMembers(); i++)
	{
		std::cout << filllevel(level + 1);
		std::cout << ref.Type()->Member(i)->Type()->Name<char>();
		std::cout << " " << ref.Type()->Member(i)->Name<char>() << " = ";
		if (ref.Type()->Member(i)->Type()->ConstType() == ::reflect::Type<const float>())
		{
			std::cout << *(const float *)ref.Member(i);
		}
		else
		{
			RAD_ASSERT(ref.Type()->Member(i)->Type()->ConstType() == ::reflect::Type<const int>());
			std::cout << *(const int *)ref.Member(i);
		}
		std::cout << std::endl;
	}

	// Output static members

	std::cout << std::endl << filllevel(level);
	std::cout << "Type " << ref.Type()->Name<char>() << " has " << ref.Type()->NumStaticMembers() << " static members." << std::endl;
	for (int i = 0; i < ref.Type()->NumStaticMembers(); i++)
	{
		std::cout << filllevel(level + 1);
		std::cout << ref.Type()->StaticMember(i)->Type()->Name<char>();
		std::cout << " " << ref.Type()->StaticMember(i)->Name<char>() << " = " << *(const float *)ref.Type()->StaticMember(i) << std::endl;
	}

	// Output static constants

	std::cout << std::endl << filllevel(level);
	std::cout << "Type " << ref.Type()->Name<char>() << " has " << ref.Type()->NumStaticConstants() << " static constants." << std::endl;
	for (int i = 0; i < ref.Type()->NumStaticConstants(); i++)
	{
		std::cout << filllevel(level + 1);
		std::cout << ref.Type()->StaticConstant(i)->Type()->Name<char>();
		std::cout << " " << ref.Type()->StaticConstant(i)->Name<char>() << " = " << *(const float *)ref.Type()->StaticConstant(i) << std::endl;
	}

	// Output methods

	std::cout << std::endl << filllevel(level);
	std::cout << "Type " << ref.Type()->Name<char>() << " has " << c->NumMethods() << " methods." << std::endl;
	for (int i = 0; i < c->NumMethods(); i++)
	{
		const reflect::Class::METHOD *m = c->Method(i);
		std::cout << filllevel(level + 1);
		if (NULL == m->ReturnType()) { std::cout << "void "; }
		else { std::cout << m->ReturnType()->Name<char>() << " "; }
		std::cout << m->Name<char>() << "(";
		for (int k = 0; k < m->NumArguments(); k++)
		{
			const reflect::Class::METHOD::ARGUMENT *a = m->Argument(k);
			if (NULL == a->Type())
			{
				RAD_ASSERT(1 == m->NumArguments());
				std::cout << "void ";
			}
			else
			{
				if (a->IsConst()) { std::cout << "const "; }
				std::cout << a->Type()->Name<char>() << " ";
				if (a->IsReference()) { std::cout << "&"; }
				std::cout << a->Name<char>();
				const void *def = a->DefaultValue();
				std::cout.precision(1);
				if (NULL != def) { std::cout << " = " << std::fixed << std::showpoint << *reinterpret_cast<const float *>(def) << "f"; }
				if (k < m->NumArguments() - 1) { std::cout << ", "; }
			}
		}
		std::cout << ");" << std::endl;
	}

	// Output const methods

	std::cout << std::endl << filllevel(level);
	std::cout << "Type " << ref.Type()->Name<char>() << " has " << c->NumConstMethods() << " const methods." << std::endl;
	for (int i = 0; i < c->NumConstMethods(); i++)
	{
		const reflect::Class::METHOD *m = c->ConstMethod(i);
		std::cout << filllevel(level + 1);
		if (NULL == m->ReturnType()) { std::cout << "void "; }
		else { std::cout << m->ReturnType()->Name<char>() << " "; }
		std::cout << m->Name<char>() << "(";
		for (int k = 0; k < m->NumArguments(); k++)
		{
			const reflect::Class::METHOD::ARGUMENT *a = m->Argument(k);
			if (NULL == a->Type())
			{
				RAD_ASSERT(1 == m->NumArguments());
				std::cout << "void ";
			}
			else
			{
				if (a->IsConst()) { std::cout << "const "; }
				std::cout << a->Type()->Name<char>() << " ";
				if (a->IsReference()) { std::cout << "&"; }
				std::cout << a->Name<char>();
				const void *def = a->DefaultValue();
				std::cout.precision(1);
				if (NULL != def) { std::cout << " = " << std::fixed << std::showpoint << *reinterpret_cast<const float *>(def) << "f"; }
				if (k < m->NumArguments() - 1) { std::cout << ", "; }
			}
		}
		std::cout << ") const;" << std::endl;
	}

	// Output static methods

	std::cout << std::endl << filllevel(level);
	std::cout << "Type " << ref.Type()->Name<char>() << " has " << c->NumStaticMethods() << " static methods." << std::endl;
	for (int i = 0; i < c->NumStaticMethods(); i++)
	{
		std::cout << filllevel(level + 1);
		const reflect::Class::METHOD *m = c->StaticMethod(i);
		std::cout << "  static ";
		if (NULL == m->ReturnType()) { std::cout << "void "; }
		else { std::cout << m->ReturnType()->Name<char>() << " "; }
		std::cout << m->Name<char>() << "(";
		for (int k = 0; k < m->NumArguments(); k++)
		{
			const reflect::Class::METHOD::ARGUMENT *a = m->Argument(k);
			if (NULL == a->Type())
			{
				RAD_ASSERT(1 == m->NumArguments());
				std::cout << "void ";
			}
			else
			{
				if (a->IsConst()) { std::cout << "const "; }
				std::cout << a->Type()->Name<char>() << " ";
				if (a->IsReference()) { std::cout << "&"; }
				std::cout << a->Name<char>();
				const void *def = a->DefaultValue();
				std::cout.precision(1);
				if (NULL != def) { std::cout << " = " << std::fixed << std::showpoint << *reinterpret_cast<const float *>(def) << "f"; }
				if (k < m->NumArguments() - 1) { std::cout << ", "; }
			}
		}
		std::cout << ");" << std::endl;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// CallGetX()
//////////////////////////////////////////////////////////////////////////////////////////

static void CallGetX(const reflect::Reflected& ref)
{
	float r;
	std::cout << "  GetX()... ";
	std::cout.precision(1);
	try
	{
		reflect::Reflected z = reflect::Reflect(r);
		reflect::NullArgs zz;
		reflect::IArgumentList &ial = zz;

		ref.FindAndCallConstMethod(
			z,
			"GetX",
			ial
		);
		std::cout << "succeeded! result = " << std::fixed << std::showpoint << r << std::endl;
	}
	catch (reflect::MethodNotFoundException &)
	{
		std::cout << "failed!" << std::endl;
	}

	const reflect::Class::CONSTMETHOD *method = ref.Type()->FindConstMethod<float ()>("GetX");
	const reflect::Class::CONSTRUCTOR *constructor = ref.Type()->FindConstructor<void ()>();
}

//////////////////////////////////////////////////////////////////////////////////////////
// CallSetX<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
static void CallSetX(const reflect::Reflected& ref, T val)
{
	try
	{
		ref.FindAndCallMethod(
			reflect::Reflected(),
			"SetX",
			reflect::FixedArgumentList<1>().PushBack(reflect::Reflect(val))
		);

		std::cout << "succeeded!" << std::endl;
	}
	catch (reflect::MethodNotFoundException &)
	{
		std::cout << "failed!" << std::endl;
	}
	catch (reflect::InvalidCastException &)
	{
		std::cout << "failed!" << std::endl;
	}

	const reflect::Class::METHOD *method = ref.FindMethod<void (T)>("GetX");
}

//////////////////////////////////////////////////////////////////////////////////////////
// CallSetXDefault()
//////////////////////////////////////////////////////////////////////////////////////////

static void CallSetXDefault(const reflect::Reflected& ref)
{
	try
	{
		ref.FindAndCallMethod(
			reflect::Reflected(),
			"SetX",
			reflect::NullArgs()
		);
		std::cout << "succeeded!" << std::endl;
	}
	catch (reflect::MethodNotFoundException &)
	{
		std::cout << "failed!" << std::endl;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// CallRefTestGetZ()
//////////////////////////////////////////////////////////////////////////////////////////

static void CallRefTestGetZ(const reflect::Reflected &ref)
{
	try
	{
		float z = 0.0f;
		ref.FindAndCallMethod(
			reflect::Reflected(),
			"RefTestGetZ",
			reflect::FixedArgumentList<1>().PushBack(reflect::Reflect(z))
		);
		std::cout << "succeeded!" << std::endl;
	}
	catch (reflect::MethodNotFoundException &)
	{
		std::cout << "failed!" << std::endl;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// CallSetEnumValue()
//////////////////////////////////////////////////////////////////////////////////////////

static void CallSetEnumValue(const reflect::Reflected &ref)
{
	try
	{
		int val = VALUE0;
		//AddrSize size = ::reflect::Type<ReflectedEnum>()->Size();
		const reflect::Class::MUTABLEMETHOD *method = ref.FindMethod("SetEnumValue", reflect::FixedTypeList<1>().PushBack(::reflect::Type<ReflectedEnum>()), false);
		if (method)
		{
			ref.CallMethod(
				reflect::Reflect(&val, ::reflect::Type<ReflectedEnum>()),
				method,
				reflect::FixedArgumentList<1>().PushBack(reflect::Reflect(&val, ::reflect::Type<ReflectedEnum>()))
			);
			std::cout << "succeeded!" << std::endl;
		}
		else
		{
			throw reflect::MethodNotFoundException();
		}
	}
	catch (reflect::MethodNotFoundException &)
	{
		std::cout << "failed!" << std::endl;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// CallSetIntValue()
//////////////////////////////////////////////////////////////////////////////////////////

static void CallSetIntValue(const reflect::Reflected &ref)
{
	try
	{
		int val = 0;
		ref.FindAndCallMethod(
			reflect::Reflected(),
			"SetIntValue",
			reflect::FixedArgumentList<1>().PushBack(reflect::Reflect(val))
		);
		std::cout << "succeeded!" << std::endl;
	}
	catch (reflect::MethodNotFoundException &)
	{
		std::cout << "failed!" << std::endl;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// CallStaticGetX()
//////////////////////////////////////////////////////////////////////////////////////////

static void CallStaticGetX(const reflect::Class *type)
{
	float r;
	std::cout << "  static GetX()... ";
	std::cout.precision(1);

	try
	{
		type->FindAndCallStaticMethod(
			reflect::Reflect(r),
			"GetX",
			reflect::NullArgs()
		);
		std::cout << "succeeded! result = " << std::fixed << std::showpoint << r << std::endl;
	}
	catch (reflect::MethodNotFoundException &)
	{
		std::cout << "failed!" << std::endl;
	}

	const reflect::Class::STATICMETHOD *method = type->FindStaticMethod<float ()>("GetX");
}

//////////////////////////////////////////////////////////////////////////////////////////
// CallStaticSetX<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
static void CallStaticSetX(const reflect::Class *type, T val)
{
	try
	{
		type->FindAndCallStaticMethod(
			reflect::Reflected(),
			"SetX",
			reflect::FixedArgumentList<1>().PushBack(reflect::Reflect(val))
		);

		std::cout << "succeeded!" << std::endl;
	}
	catch (reflect::MethodNotFoundException &)
	{
		std::cout << "failed!" << std::endl;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// CallStaticSetXDefault()
//////////////////////////////////////////////////////////////////////////////////////////

static void CallStaticSetXDefault(const reflect::Class *type)
{
	try
	{
		type->FindAndCallStaticMethod(
			reflect::Reflected(),
			"SetX",
			reflect::NullArgs()
		);
		std::cout << "succeeded!" << std::endl;
	}
	catch (reflect::MethodNotFoundException &)
	{
		std::cout << "failed!" << std::endl;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// CallMethods()
//////////////////////////////////////////////////////////////////////////////////////////

static void CallMethods(const reflect::Reflected& ref)
{
	std::cout << std::endl << "Calling methods" << std::endl;

	CallGetX(ref);
	std::cout << "  SetX((float)500.0f)... ";
	CallSetX<float>(ref, 500.0f);
	CallGetX(ref);
	std::cout << "  SetX((int)200)... ";
	CallSetX<int>(ref, 200);
	CallGetX(ref);
	std::cout << "  SetX()... ";
	CallSetXDefault(ref);
	CallGetX(ref);
	std::cout << "  RefTestGetZ((float&))... ";
	CallRefTestGetZ(ref);
	CallStaticGetX(ref.Type());
	std::cout << "  static SetX((float)333.0f)... ";
	CallStaticSetX<float>(ref.Type(), 333.0f);
	CallStaticGetX(ref.Type());
	std::cout << "  static SetX()... ";
	CallStaticSetXDefault(ref.Type());
	CallStaticGetX(ref.Type());
	CallGetX(ref);
	std::cout << "  SetEnumValue()... ";
	CallSetEnumValue(ref);
	std::cout << "  SetIntValue()... ";
	CallSetIntValue(ref);
}

//////////////////////////////////////////////////////////////////////////////////////////
// StackVarClassTest()
//////////////////////////////////////////////////////////////////////////////////////////

static void StackVarClassTest()
{
	const reflect::Class *type = reflect::Type<ReflectClassTest>();
	const reflect::Class::CONSTRUCTOR *constructor = type->FindConstructor<void ()>();
	RAD_REFLECTED_STACK_VAR_CONSTRUCTOR_ARGS_ALIGN(r, type, constructor, reflect::NullArgs(), 8);
	RAD_REFLECTED_STACK_VAR_ARGS_ALIGN(r2, type, void (), reflect::NullArgs(), 8);
	RAD_REFLECTED_STACK_VAR_ALIGN(r3, type, 8);
	RAD_REFLECTED_STACK_VAR_CONSTRUCTOR_ARGS(r4, type, constructor, reflect::NullArgs());
	RAD_REFLECTED_STACK_VAR(r5, type);
	CallMethods(r);
	CallMethods(r2);
	CallMethods(r3);
	CallMethods(r4);
	CallMethods(r5);
}

RADREFLECT_BEGIN_TYPES_EXPORT
	RADREFLECT_EXPORT_TYPE(ReflectClassTest)
RADREFLECT_END_TYPES_EXPORT

//////////////////////////////////////////////////////////////////////////////////////////
// ReflectTest::Run()
//////////////////////////////////////////////////////////////////////////////////////////

#include "../../Runtime/Tuple.h"
#include "../../Runtime/Tuple/MakeTuple.h"

namespace ut
{

void ReflectTest()
{
    ut::Begin("ReflectTest");
	ReflectClassTest rft(100, 139, 240);
	const reflect::Class *pType2 = reflect::Class::Find("ReflectClassTest");
	const reflect::Class *pType1 = reflect::Type<ReflectClassTest>();
	RAD_ASSERT(NULL != pType1);
	RAD_ASSERT(NULL != pType2);
	RAD_ASSERT(pType1 == pType2);
	DumpClassInfo(reflect::Reflect(rft));
	CallMethods(reflect::Reflect(rft));
	StackVarClassTest();
	std::cout << std::endl;

	TupleTraits<int, float>::Type tup = MakeTuple(0, 1.0f);
	float e1 = tup.Element<1>();
	int e0 = tup.Element<0>();

	{
		const int X = 14;
		const int &constIntTest = X;
		reflect::ConstReflected r = reflect::Reflect(constIntTest);
	}

	std::cout << "Printing type list:" << std::endl;
	{
		reflect::TypeEnumerator e;
		for (const reflect::Class *c = e.Begin(); c != 0; c = e.Next())
		{
			std::cout << c->Name<char>() << std::endl;
		}
	}

	std::cout << "Testing ordinal copy" << std::endl;
	{
		int x(210);

		/*reflect::SharedReflected::Ref x(
			new reflect::SharedReflected(
				reflect::Reflected::New(
					ZRuntime,
					reflect::Type<int>(),
					reflect::NullArgs()
				)
			)
		);*/

		//std::cout << *static_cast<int*>(*x) << std::endl;

		reflect::SharedReflected::Ref copy(
			new reflect::SharedReflected(
				reflect::Reflected::Clone(
					ZRuntime,
					reflect::Reflect(x)
				)
			)
		);

		std::cout << *static_cast<int*>(*copy) << std::endl;
	}
}

}
