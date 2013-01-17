/*! \file CVars.inl
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

inline CVarZone::CVarZone() : m_open(false), m_cvarSaveCount(0) {
}

///////////////////////////////////////////////////////////////////////////////

inline CVar::~CVar() {
	// you must call CVarZone::Close() before destructing any CVars you have registered.
	RAD_ASSERT(!m_zone || (m_zone == &CVarZone::Globals()));
}

inline CVar::CVar(CVarZone &zone, const char *name, Type type, bool save) 
: m_zone(&zone), m_name(name), m_type(type), m_save(save) {
	zone.Register(this);
}

///////////////////////////////////////////////////////////////////////////////

inline CVarString::CVarString(const char *name, const char *value, bool save)
: CVar(CVarZone::Globals(), name, CVar::kType_String, save), m_value(value) {
}

inline CVarString::CVarString(CVarZone &zone, const char *name, const char *value, bool save)
: CVar(zone, name, CVar::kType_String, save), m_value(value) {
}

inline String CVarString::ToString() const {
	return m_value;
}

inline bool CVarString::Parse(const char *value) {
	m_value = value;
	if (saves)
		zone->Flush();
	return true;
}

///////////////////////////////////////////////////////////////////////////////

inline CVarInt::CVarInt(const char *name, int value, bool save)
: CVar(CVarZone::Globals(), name, CVar::kType_Int, save), m_value(value) {
}

inline CVarInt::CVarInt(CVarZone &zone, const char *name, int value, bool save)
: CVar(zone, name, CVar::kType_Int, save), m_value(value) {
}

inline String CVarInt::ToString() const {
	String s;
	s.PrintfASCII("%d", m_value);
	return s;
}

inline bool CVarInt::Parse(const char *value) {
	RAD_ASSERT(value);
	sscanf(value, "%d", &m_value);
	if (saves)
		zone->Flush();
	return true;
}

///////////////////////////////////////////////////////////////////////////////

inline CVarBool::CVarBool(const char *name, bool value, bool save)
: CVar(CVarZone::Globals(), name, CVar::kType_Bool, save), m_value(value) {
}

inline CVarBool::CVarBool(CVarZone &zone, const char *name, bool value, bool save)
: CVar(zone, name, CVar::kType_Bool, save), m_value(value) {
}

inline String CVarBool::ToString() const {
	if (m_value)
		return CStr("1");
	return CStr("0");
}

///////////////////////////////////////////////////////////////////////////////

inline CVarFloat::CVarFloat(const char *name, float value, bool save)
: CVar(CVarZone::Globals(), name, CVar::kType_Float, save), m_value(value) {
}

inline CVarFloat::CVarFloat(CVarZone &zone, const char *name, float value, bool save)
: CVar(zone, name, CVar::kType_Float, save), m_value(value) {
}

inline String CVarFloat::ToString() const {
	String s;
	s.PrintfASCII("%f", m_value);
	return s;
}

inline bool CVarFloat::Parse(const char *value) {
	RAD_ASSERT(value);
	sscanf(value, "%f", &m_value);
	if (saves)
		zone->Flush();
	return true;
}

///////////////////////////////////////////////////////////////////////////////

inline CVarFunc::CVarFunc(const char *name)
: CVar(CVarZone::Globals(), name, CVar::kType_Func, false) {
}

inline CVarFunc::CVarFunc(CVarZone &zone, const char *name)
: CVar(zone, name, CVar::kType_Func, false) {
}

inline String CVarFunc::ToString() const {
	return CStr("Unsupported Method");
}

inline bool CVarFunc::Parse(const char *value) {
	return false;
}

inline bool CVarFunc::Read(stream::InputStream &is) {
	RAD_FAIL("Unsupported Method");
	return false;
}

inline bool CVarFunc::Write(stream::OutputStream &os) const {
	RAD_FAIL("Unsupported Method");
	return false;
}
