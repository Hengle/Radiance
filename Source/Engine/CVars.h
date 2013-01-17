/*! \file CVars.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#include "Types.h"
#include <Runtime/FileDef.h>
#include <Runtime/StreamDef.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

class CVar;
class CVarString;
class CVarInt;
class CVarBool;
class CVarFloat;
class CVarFunc;
class CVarZone;

typedef zone_vector<CVar*, ZEngineT>::type CVarVec;
typedef zone_map<String, CVar*, ZEngineT>::type CVarMap;

//! Defines a collection of CVars.
class CVarZone {
public:
	enum FindScope {
		kFindScope_LocalOnly,
		kFindScope_IncludingGlobals
	};
	
	CVarZone();

	void Open(const char *path);
	void Close();
	void Flush();
	CVar *Find(const char *name, FindScope scope) const;
	CVarVec StartsWith(const char *name) const;
	
	RAD_DECLARE_READONLY_PROPERTY(CVarZone, cvars, const CVarMap*);

	static CVarZone &Globals();

private:

	friend class CVar;
	friend class Engine;

	RAD_DECLARE_GET(cvars, const CVarMap*) {
		return &m_cvars;
	}

	void Load(stream::InputStream &is);
	void Save(stream::OutputStream &os);

	void Register(CVar *cvar);

	CVarMap m_cvars;
	String m_path;
	int m_cvarSaveCount;
	bool m_open;
};

///////////////////////////////////////////////////////////////////////////////

//! CVars can be set from the debug console
class CVar : public boost::noncopyable {
public:
	typedef boost::shared_ptr<CVar> Ref;

	enum Type {
		kType_String,
		kType_Int,
		kType_Bool,
		kType_Float,
		kType_Func
	};

	virtual ~CVar();
	
	virtual String ToString() const = 0;
	virtual bool Parse(const char *value) = 0;

	virtual bool Read(stream::InputStream &is) = 0;
	virtual bool Write(stream::OutputStream &os) const = 0;

	RAD_DECLARE_READONLY_PROPERTY(CVar, type, Type);
	RAD_DECLARE_READONLY_PROPERTY(CVar, name, const char*);
	RAD_DECLARE_READONLY_PROPERTY(CVar, zone, CVarZone*);
	RAD_DECLARE_READONLY_PROPERTY(CVar, saves, bool);

private:

	friend class CVarString;
	friend class CVarInt;
	friend class CVarBool;
	friend class CVarFloat;
	friend class CVarFunc;
	friend class CVarZone;

	CVar(CVarZone &zone, const char *name, Type type, bool save);
	
	RAD_DECLARE_GET(type, Type) {
		return m_type;
	}

	RAD_DECLARE_GET(name, const char*) {
		return m_name.c_str;
	}

	RAD_DECLARE_GET(zone, CVarZone*) {
		return m_zone;
	}

	RAD_DECLARE_GET(saves, bool) {
		return m_save;
	}
	
	CVarZone *m_zone;
	Type m_type;
	String m_name;
	bool m_save;
};

///////////////////////////////////////////////////////////////////////////////

class CVarString : public CVar {
public:
	static const CVar::Type kType = CVar::kType_String;
	
	CVarString(const char *name, const char *value, bool save);
	CVarString(CVarZone &zone, const char *name, const char *value, bool save);

	virtual String ToString() const;
	virtual bool Parse(const char *value);

	RAD_DECLARE_PROPERTY(CVarString, value, const String&, const String&);

protected:

	virtual bool Read(stream::InputStream &is);
	virtual bool Write(stream::OutputStream &os) const;

private:

	RAD_DECLARE_GET(value, const String&) {
		return m_value;
	}

	RAD_DECLARE_SET(value, const String&) {
		m_value = value;
	}

	String m_value;
};

///////////////////////////////////////////////////////////////////////////////

class CVarInt : public CVar {
public:

	static const CVar::Type kType = CVar::kType_Int;

	CVarInt(const char *name, int value, bool save);
	CVarInt(CVarZone &zone, const char *name, int value, bool save);

	virtual String ToString() const;
	virtual bool Parse(const char *value);

	RAD_DECLARE_PROPERTY(CVarInt, value, int, int);

protected:

	virtual bool Read(stream::InputStream &is);
	virtual bool Write(stream::OutputStream &os) const;

private:

	RAD_DECLARE_GET(value, int) {
		return m_value;
	}

	RAD_DECLARE_SET(value, int) {
		m_value = value;
	}

	int m_value;
};

///////////////////////////////////////////////////////////////////////////////

class CVarBool : public CVar {
public:

	static const CVar::Type kType = CVar::kType_Bool;

	CVarBool(const char *name, bool value, bool save);
	CVarBool(CVarZone &zone, const char *name, bool value, bool save);

	virtual String ToString() const;
	virtual bool Parse(const char *value);

	RAD_DECLARE_PROPERTY(CVarBool, value, bool, bool);

protected:

	virtual bool Read(stream::InputStream &is);
	virtual bool Write(stream::OutputStream &os) const;

private:

	RAD_DECLARE_GET(value, bool) {
		return m_value;
	}

	RAD_DECLARE_SET(value, bool) {
		m_value = value;
	}

	bool m_value;
};

///////////////////////////////////////////////////////////////////////////////

class CVarFloat : public CVar {
public:

	static const CVar::Type kType = CVar::kType_Float;

	CVarFloat(const char *name, float value, bool save);
	CVarFloat(CVarZone &zone, const char *name, float value, bool save);

	virtual String ToString() const;
	virtual bool Parse(const char *value);

	RAD_DECLARE_PROPERTY(CVarFloat, value, float, float);

protected:

	virtual bool Read(stream::InputStream &is);
	virtual bool Write(stream::OutputStream &os) const;

private:

	RAD_DECLARE_GET(value, float) {
		return m_value;
	}

	RAD_DECLARE_SET(value, float) {
		m_value = value;
	}

	float m_value;
};

///////////////////////////////////////////////////////////////////////////////

class CVarFunc : public CVar {
public:

	static const CVar::Type kType = CVar::kType_Func;

	CVarFunc(const char *name);
	CVarFunc(CVarZone &zone, const char *name);

	//! Executes the cvar, cmdline can be null.
	virtual void Execute(const char *cmdline) = 0;

	virtual String ToString() const;
	virtual bool Parse(const char *value);

protected:

	virtual bool Read(stream::InputStream &is);
	virtual bool Write(stream::OutputStream &os) const;

};

#include <Runtime/PopPack.h>
#include "CVars.inl"
