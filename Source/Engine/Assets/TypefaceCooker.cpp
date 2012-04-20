// TypefaceCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "TypefaceCooker.h"
#include "MaterialParser.h"
#include "../App.h"
#include "../Engine.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

TypefaceCooker::TypefaceCooker() : Cooker(0)
{
}

TypefaceCooker::~TypefaceCooker()
{
}

CookStatus TypefaceCooker::CheckRebuild(int flags, int allflags)
{
	if (CompareVersion(flags) || CompareModifiedTime(flags))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

CookStatus TypefaceCooker::Status(int flags, int allflags)
{
	flags &= P_AllTargets;
	allflags &= P_AllTargets;

	if (flags == 0)
	{ // only build generics if all platforms are identical to eachother.
		if (MatchTargetKeys(allflags, allflags)==allflags)
			return CheckRebuild(flags, allflags);
		return CS_Ignore;
	}

	if (MatchTargetKeys(allflags, allflags)==allflags)
		return CS_Ignore;

	// only build ipad if different from iphone
	if ((flags&P_TargetIPad) && (allflags&P_TargetIPhone))
	{
		if (MatchTargetKeys(P_TargetIPad, P_TargetIPhone))
			return CS_Ignore;
	}

	return CheckRebuild(flags, allflags);
}

int TypefaceCooker::Compile(int flags, int allflags)
{
	int pflags = flags;

	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);

	// Typeface's are not K_Global, so they have paths
	// like Source.Font.PC, etc. So if flags == 0 then
	// we are doing generics (i.e. all targets have the same
	// values), however to load the actual data we can't use
	// a flags value of zero, since that will return the Typeface.keys
	// defaults which have blanks (and aren't what the user set anyway).

	if ((flags&P_AllTargets)==0)
	{ // use any target, since generics means all targets are the same.
		flags = (flags&~P_AllTargets)|FirstTarget(allflags);
	}

	const String *s = asset->entry->KeyValue<String>("Source.Font", flags);
	if (!s || s->empty())
		return SR_MetaError;

	// import font.
	AddImport(s->c_str(), pflags);

	s = asset->entry->KeyValue<String>("Source.Material", flags);
	if (!s || s->empty())
		return SR_MetaError;

	Asset::Ref matRef = App::Get()->engine->sys->packages->Resolve(s->c_str(), asset->zone);
	if (!matRef)
		return SR_MissingFile;

	int r = matRef->Process(
		xtime::TimeSlice::Infinite,
		flags|P_Info|P_TargetDefault
	);

	if (r != SR_Success)
		return r;

	MaterialParser::Ref matParser = MaterialParser::Cast(matRef);
	if (!matParser)
		return SR_MetaError;
	if (!matParser->procedural)
	{
		cout.get() << "ERROR: Materials referenced by a typeface must be marked as procedural!" << std::endl;
		return SR_MetaError;
	}
	
	// import material
	AddImport(s->c_str(), pflags);

	int w, h;

	const int *i = asset->entry->KeyValue<int>("Typeface.Width", flags);
	if (!i)
		return SR_MetaError;
	w = *i;

	i = asset->entry->KeyValue<int>("Typeface.Height", flags);
	if (!i)
		return SR_MetaError;
	h = *i;

	if (w < 1 || h < 1)
		return SR_MetaError;

	// save width/height to tag
	BinFile::Ref f = OpenTagWrite(flags);
	if (!f)
		return SR_IOError;

	stream::OutputStream os(f->ob);

	os << (U16)w << (U16)h;

	return SR_Success;
}

int TypefaceCooker::MatchTargetKeys(int flags, int allflags)
{
	return asset->entry->MatchTargetKeys<String>("Source.Font", flags, allflags)&
		asset->entry->MatchTargetKeys<String>("Source.Material", flags, allflags)&
		asset->entry->MatchTargetKeys<int>("Typeface.Width", flags, allflags)&
		asset->entry->MatchTargetKeys<int>("Typeface.Height", flags, allflags);
}

void TypefaceCooker::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<TypefaceCooker>();
}

} // asset
