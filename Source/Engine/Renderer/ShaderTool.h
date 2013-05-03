/*! \file ShaderTool.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#pragma once

#if !defined(RAD_OPT_TOOLS)
	#error "This file should only be included in tools builds"
#endif

#include "../Types.h"
#include "../Lua/LuaRuntime.h"
#include "Common.h"
#include "Shader.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Container/ZoneVector.h>
#include <iostream>
#include <Runtime/PushPack.h>

class Engine;

namespace r {
	class Material;	
}

namespace tools {
namespace shader_utils {

//! Shader construction class.
/*! Radiance shaders produce GLSL, and HLSL from a set of shader files which describe a node
    graph. Each node represents a fragment of shader code. 
	
	This class parses a shader file and compiles this graph into an in-memory representation, 
	and provides facilities for emiting shader code. This class is totally agnostic to any
	particular shader target. The shader-code fragments contained in each node are instead
	heavily macro laden making them capable of compiling for both HLSL and GLSL. 

	The rendering backend defines a set of macros that cause the agnostic shader code to
	express that targets specific keywords etc.
*/
class RADENG_CLASS Shader {
public:
	~Shader();
	typedef boost::shared_ptr<Shader> Ref;

	typedef zone_set<int, ZToolsT>::type IntSet;
	typedef zone_vector<std::pair<int, int>, ZToolsT>::type TexCoordMapping;

	//////////////////////////////////////////////////////////////////////////////////////////

	class Input;
	typedef boost::shared_ptr<Input> InputRef;
	class Output;
	typedef boost::shared_ptr<Output> OutputRef;
	typedef boost::weak_ptr<Output> OutputWRef;
	class Node;
	typedef boost::shared_ptr<Node> NodeRef;
	typedef boost::weak_ptr<Node> NodeWRef;

	enum MaterialSource {
		kMaterialSource_Node,
		kMaterialSource_Texture,
		kMaterialSource_Framebuffer,
		kMaterialSource_Color, // constant
		kMaterialSource_LightDiffuseColor, // constant (rgb) + w == brightness
		kMaterialSource_LightSpecularColor, // constant (rgb) + w == exponent
		kMaterialSource_LightPos, // model space constant (xyz) + w == radius
		kMaterialSource_LightHalfPos, // model space constant (xyz)
		kMaterialSource_Vertex,
		kMaterialSource_Normal,
		kMaterialSource_Tangent,
		kMaterialSource_Bitangent,
		kMaterialSource_LightDir, // in texture space
		kMaterialSource_LightHalfDir, // in texture space
		kMaterialSource_TexCoord,
		kMaterialSource_VertexColor,
		kMaterialSource_SpriteSkin,
		kNumMaterialSources
	};

	enum BasicType {
		kBasicType_Float,
		kBasicType_Float2,
		kBasicType_Float3,
		kBasicType_Float4,
		kBasicType_Half,
		kBasicType_Half2,
		kBasicType_Half3,
		kBasicType_Half4,
		kBasicType_Fixed,
		kBasicType_Fixed2,
		kBasicType_Fixed3,
		kBasicType_Fixed4,
		kBasicType_Sampler2D,
		kBasicType_SamplerCUBE,
		kNumBasicTypes
	};

	enum SkinMode {
		kSkinMode_Default,
		kSkinMode_Sprite
	};

	enum ShaderOutput {
		kShaderOutput_Color,
		kShaderOutput_Alpha,
		kShaderOutput_Depth,
		kNumShaderOutputs
	};

	enum ShaderOutputFlags {
		kShaderOutputFlag_Color = (1<<kShaderOutput_Color),
		kShaderOutputFlag_Alpha = (1<<kShaderOutput_Alpha),
		kShaderOutputFlag_Depth = (1<<kShaderOutput_Depth)
	};

	//////////////////////////////////////////////////////////////////////////////////////////

	//! A connection defines basic information about an input or output pin on a shader node.
	/*! The semantic of the pin defines the output "register" that is written to if the pin
	    is assigned to the output fields of the final shader. */
	class Connection {
	public:
		typedef boost::shared_ptr<Connection> Ref;

		RAD_DECLARE_READONLY_PROPERTY(Connection, name, const char *);
		RAD_DECLARE_READONLY_PROPERTY(Connection, type, BasicType);
		RAD_DECLARE_READONLY_PROPERTY(Connection, semantic, const char *);
		RAD_DECLARE_READONLY_PROPERTY(Connection, node, NodeRef);
		RAD_DECLARE_READONLY_PROPERTY(Connection, r, int);
		
	protected:

		Connection() : m_r(0), m_type(kNumBasicTypes) {}

	private:

		friend class Input;
		friend class Output;
		friend class Shader;

		RAD_DECLARE_GET(name, const char *);
		RAD_DECLARE_GET(type, BasicType);
		RAD_DECLARE_GET(semantic, const char *);
		RAD_DECLARE_GET(node, NodeRef);
		RAD_DECLARE_GET(r, int);
		
		String m_name;
		BasicType m_type;
		String m_semantic;
		NodeWRef m_node;
		int m_r;
	};

	//////////////////////////////////////////////////////////////////////////////////////////

	class Input : public Connection, public boost::enable_shared_from_this<Input> {
	public:
		typedef InputRef Ref;
		Input() : m_msource(kMaterialSource_Node), m_msourceIdx(0) {}
		
		RAD_DECLARE_PROPERTY(Input, source, OutputRef, OutputRef);
		RAD_DECLARE_PROPERTY(Input, msource, MaterialSource, MaterialSource);
		RAD_DECLARE_PROPERTY(Input, msourceIdx, int, int);
		
	private:

		RAD_DECLARE_GET(source, OutputRef);
		RAD_DECLARE_SET(source, OutputRef);
		RAD_DECLARE_GET(msource, MaterialSource);
		RAD_DECLARE_SET(msource, MaterialSource);
		RAD_DECLARE_GET(msourceIdx, int);
		RAD_DECLARE_SET(msourceIdx, int);
		
		OutputWRef m_source;
		MaterialSource m_msource;
		int m_msourceIdx;
	};

	//////////////////////////////////////////////////////////////////////////////////////////

	class Output : public Connection, public boost::enable_shared_from_this<Output> {
	public:
		typedef OutputRef Ref;
		typedef OutputWRef WRef;

		void Connect(const InputRef &dest);
		void Disconnect(const InputRef &dest);
		void DisconnectAll();
		
	private:

		struct _X {
			InputRef iref;
			NodeRef  nref;
		};

		typedef zone_map<Input*, _X, ZToolsT>::type PtrMap;

		PtrMap m_inputs;
	};

	//////////////////////////////////////////////////////////////////////////////////////////

	class Node : public boost::enable_shared_from_this<Node> {
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
	RAD_DECLARE_READONLY_PROPERTY(Shader, skinMode, SkinMode);

	static Ref Load(Engine &engine, const char *name);

	//! Emits supporting shader functions.
	bool EmitFunctions(Engine &engine, std::ostream &out) const;
	
	//! Emits shader code.
	/*! Shader code is generated in 2 passes. First call EmitFunnctions to emit
	    supporting shader functions and data. Then call this function to emit the
		shader's MAIN entry point. */
	bool EmitShader(
		const char *fnName, 
		r::Shader::Pass pass,
		const r::Material &m,
		std::ostream &out
	) const;

	//! Returns true if the specified pass exists in the shader.
	bool Exists(r::Shader::Pass pass) const;

	//! Returns ShaderOutputFlags for fields written to by shader pass.
	int PassOutputs(r::Shader::Pass pass) const;

	//! Returns the usage of the specified material inputs.
	int MaterialSourceUsage(r::Shader::Pass pass, MaterialSource source) const;

	//! Returns the mapping from input register to attribute array based on material.
	bool BuildInputMappings(
		const r::Material &material, 
		r::Shader::Pass pass,
		r::MaterialInputMappings &mapping,
		TexCoordMapping &tcMapping
	) const;

	//! Returns attribute usage map.
	const IntSet &AttributeUsage(r::Shader::Pass pass, MaterialSource source);

private:
	Shader(const char *name);
	
	typedef zone_map<String, Node::Ref, ZToolsT>::type NodeMap;
	typedef zone_vector<Node::Ref, ZToolsT>::type NodeVec;
	typedef zone_map<void*, int, ZToolsT>::type VarMap;

	struct Usage {
		boost::array<IntSet, kNumMaterialSources> s;
	};

	class ImportLoader : public lua::ImportLoader {
	public:
		virtual lua::SrcBuffer::Ref Load(lua_State *L, const char *name);
	};

	static int lua_SkinMode(lua_State *L);
	static int lua_MNode(lua_State *L);
	static int lua_NNode(lua_State *L);
	static int lua_Compile(lua_State *L);
	static int lua_MColor(lua_State *L);
	static int lua_MTexture(lua_State *L);
	static int lua_MFramebuffer(lua_State *L);
	static int lua_MTexCoord(lua_State *L);
	static int lua_MVertex(lua_State *L);
	static int lua_MNormal(lua_State *L);
	static int lua_MTangent(lua_State *L);
	static int lua_MBitangent(lua_State *L);
	static int lua_MLightPos(lua_State *L);
	static int lua_MLightHalfPos(lua_State *L);
	static int lua_MLightDiffuseColor(lua_State *L);
	static int lua_MLightSpecularColor(lua_State *L);
	static int lua_MLightDir(lua_State *L);
	static int lua_MLightHalfDir(lua_State *L);
	static int lua_MVertexColor(lua_State *L);
	static int lua_MSource(lua_State *L, MaterialSource source);
	static int lua_gcNode(lua_State *L);
	static void ParseConnection(lua_State *L, Node *node, const lua::Variant::Map &map, Connection &c);
	static lua::State::Ref InitLuaM(Engine &e, Shader *m);
	static lua::State::Ref InitLuaN(Node *n);

	RAD_DECLARE_GET(name, const char*);
	RAD_DECLARE_GET(skinMode, SkinMode);

	Node::Ref LoadNode(Engine &e, lua_State *L, const char *type);
	
	void ParseShaderPass(
		lua_State *L,
		const char *name, 
		r::Shader::Pass pass,
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

	bool szMaterialInput(
		char *out, 
		r::Shader::Pass pass, 
		const TexCoordMapping &tcMapping,
		MaterialSource source, 
		int index
	) const;

	void BuildInputMappings(
		lua_State *L, 
		r::Shader::Pass pass
	);
	
	void BuildTextureSourceMapping(
		lua_State *L,
		int &num, 
		r::MaterialTextureSource source,
		const IntSet &usage,
		r::MaterialInputMappings &mapping
	);

	void BuildAttributeSourceMapping(
		lua_State *L,
		int &num,
		r::MaterialGeometrySource source,
		const IntSet &usage,
		r::MaterialInputMappings &mapping
	);

	bool EmitOutput(
		r::Shader::Pass pass,
		const TexCoordMapping &tcMapping,
		ShaderOutput index,
		const char *semantic,
		VarMap &vars,
		int &varOfs,
		std::ostream &out
	) const;

	bool EmitShaderNode(
		r::Shader::Pass pass,
		const TexCoordMapping &tcMapping,
		const Node::Ref &node,
		VarMap &vars,
		int &varOfs,
		std::ostream &out
	) const;

	int TextureUsageIndex(
		r::Shader::Pass pass, 
		MaterialSource source, 
		int index
	) const;

	int AttribUsageIndex(
		r::Shader::Pass pass, 
		MaterialSource source, 
		int index
	) const;

	int TexCoordUsageIndex(
		r::Shader::Pass pass,
		const TexCoordMapping &tcMapping,
		MaterialSource source, 
		int index
	) const;

	//! Returns texture coordinate register assignments for the specified material and shader pass
	TexCoordMapping BuildTexCoordMapping(const r::Material &m, r::Shader::Pass pass) const;

	struct OutNode {
		Node::Ref node;
		Input::Ref input;
	};

	struct Root {
		boost::array<OutNode, kNumShaderOutputs> out;
	};

	boost::array<Usage, r::Shader::kNumPasses> m_usage;
	boost::array<Root, r::Shader::kNumPasses> m_nodes;
	boost::array<r::MaterialInputMappings, r::Shader::kNumPasses> m_mapping;
	NodeMap m_types;
	NodeVec m_instances;
	ImportLoader m_impLoader;
	String m_name;
	SkinMode m_skinMode;
};

//////////////////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS ShaderCache {
public:
	typedef boost::shared_ptr<ShaderCache> Ref;
	typedef zone_map<String, Shader::Ref, ZToolsT>::type ShaderMap;

	RAD_DECLARE_READONLY_PROPERTY(ShaderCache, shaders, const ShaderMap&);

	Shader::Ref Load(Engine &engine, const char *name);

private:

	RAD_DECLARE_GET(shaders, const ShaderMap&) { return m_shaders; }

	ShaderMap m_shaders;
};

} // shader_utils
} // tools

#include <Runtime/PopPack.h>
#include "ShaderTool.inl"
