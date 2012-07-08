// Shader.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace cg {

inline const char *Shader::Connection::RAD_IMPLEMENT_GET(name)
{
	return m_name.c_str;
}

inline Shader::Ordinal Shader::Connection::RAD_IMPLEMENT_GET(type)
{
	return m_type;
}

inline const char *Shader::Connection::RAD_IMPLEMENT_GET(semantic)
{
	return m_semantic.c_str;
}

inline Shader::NodeRef Shader::Connection::RAD_IMPLEMENT_GET(node)
{
	return m_node.lock();
}

inline int Shader::Connection::RAD_IMPLEMENT_GET(r)
{
	return m_r;
}

//////////////////////////////////////////////////////////////////////////////////////////

inline Shader::OutputRef Shader::Input::RAD_IMPLEMENT_GET(source)
{
	return m_source.lock();
}

inline void Shader::Input::RAD_IMPLEMENT_SET(source) (OutputRef ref)
{
	OutputRef x = source;
	if (x)
		x->Disconnect(shared_from_this());
	m_source = ref;
	m_msourceIdx = 0;
	m_msource = MS_Node;
}

inline Shader::MSource Shader::Input::RAD_IMPLEMENT_GET(msource)
{
	return m_msource;
}

inline void Shader::Input::RAD_IMPLEMENT_SET(msource) (MSource ms)
{
	if (ms != MS_Node)
		this->source = OutputRef();
	m_msource = ms;
}

inline int Shader::Input::RAD_IMPLEMENT_GET(msourceIdx)
{
	return m_msourceIdx;
}

inline void Shader::Input::RAD_IMPLEMENT_SET(msourceIdx) (int idx)
{
	m_msourceIdx = idx;
}

//////////////////////////////////////////////////////////////////////////////////////////

inline void Shader::Output::Connect(const InputRef &ref)
{ 
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

inline void Shader::Output::Disconnect(const InputRef &ref)
{
	RAD_ASSERT(ref);
	m_inputs.erase(ref.get());
}

inline void Shader::Output::DisconnectAll()
{
	m_inputs.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////

inline Shader::Node::Node()
{
}

inline Shader::Node::~Node()
{
}

inline const Shader::Node::InputList *Shader::Node::RAD_IMPLEMENT_GET(inputs)
{
	return &m_inputs;
}

inline const Shader::Node::OutputList *Shader::Node::RAD_IMPLEMENT_GET(outputs)
{
	return &m_outputs;
}

inline const char *Shader::Node::RAD_IMPLEMENT_GET(name)
{
	return m_name.c_str;
}

inline const char *Shader::Node::RAD_IMPLEMENT_GET(type)
{
	return m_type.c_str;
}

inline const char *Shader::Node::RAD_IMPLEMENT_GET(alias)
{
	return m_alias.empty ? 0 : m_alias.c_str.get();
}

//////////////////////////////////////////////////////////////////////////////////////////

inline Shader::Shader(const char *name) : m_name(name)
{
}

inline Shader::~Shader()
{
}

inline bool Shader::Exists(Channel channel) const
{
	RAD_ASSERT(channel >= C_First && channel < C_Max);
	return m_nodes[channel].out[O_Color].node ||
		m_nodes[channel].out[O_Depth].node;
}

inline int Shader::TextureUsage(Channel channel) const
{
	RAD_ASSERT(channel >= C_First && channel < C_Max);
	return (int)(m_usage[channel].s[MS_Texture].size() +
		m_usage[channel].s[MS_Framebuffer].size());
}

inline int Shader::TexCoordUsage(Channel channel) const
{
	RAD_ASSERT(channel >= C_First && channel < C_Max);
	return (int)(m_usage[channel].s[MS_TexCoord].size());
}

inline int Shader::ColorUsage(Channel channel) const
{
	RAD_ASSERT(channel >= C_First && channel < C_Max);
	return 0;
}

inline int Shader::NormalUsage(Channel channel) const
{
	RAD_ASSERT(channel >= C_First && channel < C_Max);
	return 0;
}

inline int Shader::ChannelOutputs(Channel channel) const
{
	return ((m_nodes[channel].out[O_Color].node||m_nodes[channel].out[O_Color].input) ? OF_Color : 0) |
		((m_nodes[channel].out[O_Depth].node||m_nodes[channel].out[O_Depth].input) ? OF_Depth : 0);
}

inline r::GLState::MInputMappings Shader::InputMappings(Channel channel) const
{
	RAD_ASSERT(channel >= 0 && channel < C_Max);
	return m_mapping[channel];
}

inline const char *Shader::RAD_IMPLEMENT_GET(name)
{
	return m_name.c_str;
}

//////////////////////////////////////////////////////////////////////////////////////////

inline Shader::Ref ShaderCache::Load(Engine &engine, const char *name)
{
	RAD_ASSERT(name);
	String sname(name);
	ShaderMap::const_iterator it = m_shaders.find(sname);
	if (it != m_shaders.end())
		return it->second;

	Shader::Ref m = Shader::Load(engine, name);
	if (m)
	{
		m_shaders[sname] = m;
	}

	return m;
}

} // cg
