// CGShader.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if !defined(RAD_OPT_TOOLS)
	#error "This file should only be included in tools builds"
#endif

#include "../../Types.h"
#include "../../Lua/LuaRuntime.h"
#include "../GL/GLState.h"
#include "../Shader.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Container/ZoneVector.h>
#include <iostream>
#include <Runtime/PushPack.h>

class Engine;

namespace cg {

class RADENG_CLASS Shader
{
public:
	~Shader();
	typedef boost::shared_ptr<Shader> Ref;

	//////////////////////////////////////////////////////////////////////////////////////////

	class Input;
	typedef boost::shared_ptr<Input> InputRef;
	class Output;
	typedef boost::shared_ptr<Output> OutputRef;
	typedef boost::weak_ptr<Output> OutputWRef;
	class Node;
	typedef boost::shared_ptr<Node> NodeRef;
	typedef boost::weak_ptr<Node> NodeWRef;

	//////////////////////////////////////////////////////////////////////////////////////////
	
	enum Channel
	{
		C_First = r::Shader::P_Default,
		C_Default = C_First,
		C_Max
	};

	enum MSource
	{
		MS_Node,
		MS_Color,
		MS_Texture,
		MS_Framebuffer,
		MS_TexCoord,
		MS_Vertex,
		MS_Max
	};

	enum OutputFlags
	{
		OF_Color = r::Shader::OF_Color,
		OF_Depth = r::Shader::OF_Depth
	};

	enum Ordinal
	{
		Float,
		Float2,
		Float3,
		Float4,
		Half,
		Half2,
		Half3,
		Half4,
		Fixed,
		Fixed2,
		Fixed3,
		Fixed4,
		Sampler2D,
		SamplerCUBE,
		OrdMax
	};

	//////////////////////////////////////////////////////////////////////////////////////////

	class Connection
	{
	public:
		typedef boost::shared_ptr<Connection> Ref;

		RAD_DECLARE_READONLY_PROPERTY(Connection, name, const char *);
		RAD_DECLARE_READONLY_PROPERTY(Connection, type, Ordinal);
		RAD_DECLARE_READONLY_PROPERTY(Connection, semantic, const char *);
		RAD_DECLARE_READONLY_PROPERTY(Connection, node, NodeRef);
		RAD_DECLARE_READONLY_PROPERTY(Connection, r, int);
		
	protected:

		Connection() : m_r(0), m_type(OrdMax) {}

	private:

		friend class Input;
		friend class Output;
		friend class Shader;

		RAD_DECLARE_GET(name, const char *);
		RAD_DECLARE_GET(type, Ordinal);
		RAD_DECLARE_GET(semantic, const char *);
		RAD_DECLARE_GET(node, NodeRef);
		RAD_DECLARE_GET(r, int);
		
		String m_name;
		Ordinal m_type;
		String m_semantic;
		NodeWRef m_node;
		int m_r;
	};

	//////////////////////////////////////////////////////////////////////////////////////////

	class Input : public Connection, public boost::enable_shared_from_this<Input>
	{
	public:
		typedef InputRef Ref;
		Input() : m_msource(MS_Node), m_msourceIdx(0) {}
		
		RAD_DECLARE_PROPERTY(Input, source, OutputRef, OutputRef);
		RAD_DECLARE_PROPERTY(Input, msource, MSource, MSource);
		RAD_DECLARE_PROPERTY(Input, msourceIdx, int, int);
		
	private:

		RAD_DECLARE_GET(source, OutputRef);
		RAD_DECLARE_SET(source, OutputRef);
		RAD_DECLARE_GET(msource, MSource);
		RAD_DECLARE_SET(msource, MSource);
		RAD_DECLARE_GET(msourceIdx, int);
		RAD_DECLARE_SET(msourceIdx, int);
		
		OutputWRef m_source;
		MSource m_msource;
		int m_msourceIdx;
	};

	//////////////////////////////////////////////////////////////////////////////////////////

	class Output : public Connection, public boost::enable_shared_from_this<Output>
	{
	public:
		typedef OutputRef Ref;
		typedef OutputWRef WRef;

		void Connect(const InputRef &dest);
		void Disconnect(const InputRef &dest);
		void DisconnectAll();
		
	private:

		struct _X
		{
			InputRef iref;
			NodeRef  nref;
		};

		typedef zone_map<Input*, _X, ZToolsT>::type PtrMap;

		PtrMap m_inputs;
	};

	//////////////////////////////////////////////////////////////////////////////////////////

	class Node : public boost::enable_shared_from_this<Node>
	{
	public:
		~Node();
		typedef boost::shared_ptr<Node> Ref;
		typedef zone_vector<InputRef, ZToolsT>::type InputList;
		typedef zone_vector<OutputRef, ZToolsT>::type OutputList;

		InputRef FindInputName(const char *name);
		InputRef FindInputSemantic(const char *semantic);
		InputRef FindInputRegister(int r);
		OutputRef FindOutputName(const char *name);
		OutputRef FindOutputSemantic(const char *semantic);
		OutputRef FindOutputRegister(int r);

		RAD_DECLARE_READONLY_PROPERTY(Node, inputs, const InputList *);
		RAD_DECLARE_READONLY_PROPERTY(Node, outputs, const OutputList *);
		RAD_DECLARE_READONLY_PROPERTY(Node, name, const char *);
		RAD_DECLARE_READONLY_PROPERTY(Node, type, const char *);
		RAD_DECLARE_READONLY_PROPERTY(Node, alias, const char *);
		
	private:
		friend class Shader;

		Node();
		// clones everything but connections.
		Node::Ref Clone() const;

		RAD_DECLARE_GET(inputs, const InputList*);
		RAD_DECLARE_GET(outputs, const OutputList*);
		RAD_DECLARE_GET(name, const char *);
		RAD_DECLARE_GET(type, const char *);
		RAD_DECLARE_GET(alias, const char*);
		
		InputList m_inputs;
		OutputList m_outputs;
		String m_name;
		String m_type;
		String m_alias;
	};

	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_DECLARE_READONLY_PROPERTY(Shader, name, const char*);

	static Ref Load(Engine &engine, const char *name);
	bool EmitFunctions(Engine &engine, std::ostream &out) const;
	
	bool EmitShader(
		const char *fnName, 
		Channel channel, 
		std::ostream &out
	) const;

	bool Exists(Channel channel) const;

	int TextureUsage(Channel channel) const;
	int TexCoordUsage(Channel channel) const;
	int ColorUsage(Channel channel) const;
	int NormalUsage(Channel channel) const;
	int ChannelOutputs(Channel channel) const;
	r::GLState::MInputMappings InputMappings(Channel channel) const;

private:
	Shader(const char *name);
	
	enum OutputIndex
	{
		O_Color,
		O_Alpha,
		O_Depth,
		O_Max
	};

	typedef zone_map<String, Node::Ref, ZToolsT>::type NodeMap;
	typedef zone_vector<Node::Ref, ZToolsT>::type NodeVec;
	typedef zone_map<void*, int, ZToolsT>::type VarMap;

	typedef zone_set<int, ZToolsT>::type IntSet;
	struct Usage
	{
		IntSet s[MS_Max];
	};

	static int lua_MNode(lua_State *L);
	static int lua_NNode(lua_State *L);
	static int lua_Compile(lua_State *L);
	static int lua_MColor(lua_State *L);
	static int lua_MTexture(lua_State *L);
	static int lua_MTexCoord(lua_State *L);
	static int lua_MFramebuffer(lua_State *L);
	static int lua_MSource(lua_State *L, MSource source);
	static int lua_gcNode(lua_State *L);
	static void ParseConnection(lua_State *L, Node *node, const lua::Variant::Map &map, Connection &c);
	static lua::State::Ref InitLuaM(Engine &e, Shader *m);
	static lua::State::Ref InitLuaN(Node *n);

	RAD_DECLARE_GET(name, const char*);

	Node::Ref LoadNode(Engine &e, lua_State *L, const char *type);
	
	void ParseChannel(
		lua_State *L,
		const char *name, 
		Channel channel,
		const lua::Variant::Map &map
	);

	Node::Ref ParseNodes(
		lua_State *L,
		const Input::Ref &input,
		const lua::Variant::Map &map,
		NodeMap &stack
	);

	void GatherUsage(
		const Node::Ref &root,
		Usage &usage
	);

	void AddUsage(
		const Input::Ref &input,
		Usage &usage
	);

	bool szMaterialInput(char *out, Channel channel, MSource source, int index) const;

	void BuildInputMappings(lua_State *L, Channel channel);
	
	void BuildTextureSourceMapping(
		lua_State *L,
		int &num, 
		r::MTSource source,
		const IntSet &usage,
		r::GLState::MInputMappings &mapping
	);

	void BuildAttributeSourceMapping(
		lua_State *L,
		int &num,
		r::MGSource source,
		const IntSet &usage,
		r::GLState::MInputMappings &mapping
	);

	bool EmitOutput(
		Channel channel,
		OutputIndex index,
		const char *semantic,
		VarMap &vars,
		int &varOfs,
		std::ostream &out
	) const;

	bool EmitShaderNode(
		Channel channel,
		const Node::Ref &node,
		VarMap &vars,
		int &varOfs,
		std::ostream &out
	) const;

	int TextureUsageIndex(Channel channel, MSource source, int index) const;
	int AttribUsageIndex(Channel channel, MSource source, int index) const;

	struct OutNode
	{
		Node::Ref node;
		Input::Ref input;
	};

	struct Root
	{
		OutNode out[O_Max];
	};

	Usage m_usage[C_Max];
	Root m_nodes[C_Max];
	NodeMap m_types;
	NodeVec m_instances;
	r::GLState::MInputMappings m_mapping[C_Max];
	String m_name;
};

//////////////////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS ShaderCache
{
public:
	typedef boost::shared_ptr<ShaderCache> Ref;
	typedef zone_map<String, Shader::Ref, ZToolsT>::type ShaderMap;

	RAD_DECLARE_READONLY_PROPERTY(ShaderCache, shaders, const ShaderMap&);

	Shader::Ref Load(Engine &engine, const char *name);
	void Discover(Engine &engine);


private:

	RAD_DECLARE_GET(shaders, const ShaderMap&) { return m_shaders; }

	ShaderMap m_shaders;
};

} // cg

#include <Runtime/PopPack.h>
#include "CGShader.inl"
