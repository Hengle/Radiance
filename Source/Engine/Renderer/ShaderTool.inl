/*! \file ShaderTool.inl
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

namespace tools {
namespace shader_utils {

inline const char *Shader::Connection::RAD_IMPLEMENT_GET(name) {
	return m_name.c_str;
}

inline Shader::BasicType Shader::Connection::RAD_IMPLEMENT_GET(type) {
	return m_type;
}

inline const char *Shader::Connection::RAD_IMPLEMENT_GET(semantic) {
	return m_semantic.c_str;
}

inline Shader::NodeRef Shader::Connection::RAD_IMPLEMENT_GET(node) {
	return m_node.lock();
}

inline int Shader::Connection::RAD_IMPLEMENT_GET(r) {
	return m_r;
}

//////////////////////////////////////////////////////////////////////////////////////////

inline Shader::OutputRef Shader::Input::RAD_IMPLEMENT_GET(source) {
	return m_source.lock();
}

inline void Shader::Input::RAD_IMPLEMENT_SET(source) (OutputRef ref) {
	OutputRef x = source;
	if (x)
		x->Disconnect(shared_from_this());
	m_source = ref;
	m_msourceIdx = 0;
	m_msource = kMaterialSource_Node;
}

inline Shader::MaterialSource Shader::Input::RAD_IMPLEMENT_GET(msource) {
	return m_msource;
}

inline void Shader::Input::RAD_IMPLEMENT_SET(msource) (MaterialSource ms) {
	if (ms != kMaterialSource_Node)
		this->source = OutputRef();
	m_msource = ms;
}

inline int Shader::Input::RAD_IMPLEMENT_GET(msourceIdx) {
	return m_msourceIdx;
}

inline void Shader::Input::RAD_IMPLEMENT_SET(msourceIdx) (int idx) {
	m_msourceIdx = idx;
}

//////////////////////////////////////////////////////////////////////////////////////////

inline void Shader::Output::Connect(const InputRef &ref) { 
	RAD_ASSERT(ref);
	PtrMap::iterator it = m_inputs.find(ref.get());
	if (it != m_inputs.end())
		return;

	// make sure to grab the node first before we set our source
	// cause the input will unmap its current source which could
	// delete it.
	
	_X x;
	x.nref = ref->node;
	x.iref = ref;
	ref->source = shared_from_this();
	m_inputs[ref.get()] = x;
}

inline void Shader::Output::Disconnect(const InputRef &ref) {
	RAD_ASSERT(ref);
	m_inputs.erase(ref.get());
}

inline void Shader::Output::DisconnectAll() {
	m_inputs.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////

inline Shader::Node::Node() {
}

inline Shader::Node::~Node() {
}

inline const Shader::Node::InputList *Shader::Node::RAD_IMPLEMENT_GET(inputs) {
	return &m_inputs;
}

inline const Shader::Node::OutputList *Shader::Node::RAD_IMPLEMENT_GET(outputs) {
	return &m_outputs;
}

inline const char *Shader::Node::RAD_IMPLEMENT_GET(name) {
	return m_name.c_str;
}

inline const char *Shader::Node::RAD_IMPLEMENT_GET(type) {
	return m_type.c_str;
}

inline const char *Shader::Node::RAD_IMPLEMENT_GET(alias) {
	return m_alias.empty ? 0 : m_alias.c_str.get();
}

//////////////////////////////////////////////////////////////////////////////////////////

inline Shader::~Shader() {
}

inline int Shader::MaterialSourceUsage(r::Shader::Pass pass, MaterialSource source) const {
	return (int)m_usage[pass].s[source].size();
}

inline const Shader::IntSet &Shader::AttributeUsage(r::Shader::Pass pass, MaterialSource source) {
	return m_usage[pass].s[source];
}

inline const char *Shader::RAD_IMPLEMENT_GET(name) {
	return m_name.c_str;
}

} // shader_utils
} // tool
