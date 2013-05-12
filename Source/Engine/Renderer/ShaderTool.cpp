/*! \file ShaderTool.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#include RADPCH

#include "Material.h"
#include "ShaderTool.h"
#include "ShaderToolUtils.h"
#include "../Lua/LuaRuntime.h"
#include "../COut.h"
#include "../App.h"
#include "../Engine.h"
#include <Runtime/File.h>

#define SELF "@t"
#define ENG "@e"
#define META "@m"
#define GCT "@g"

namespace tools {
namespace shader_utils {

namespace {

enum NodeType {
	kNodeType_Frag,
	kNodeType_MaterialSource
};

const char *s_types[Shader::kNumBasicTypes] = {
	"FLOAT",
	"FLOAT2",
	"FLOAT3",
	"FLOAT4",
	"HALF",
	"HALF2",
	"HALF3",
	"HALF4",
	"FIXED",
	"FIXED2",
	"FIXED3",
	"FIXED4",
	"sampler2D",
	"samplerCUBE"
};

void RegisterBasicTypes(lua_State *L) {
	for (int i = 0; i < Shader::kNumBasicTypes; ++i) {
		lua_pushnumber(L, (lua_Number)i);
		lua_setglobal(L, s_types[i]);
	}
}

} // namespace

lua::SrcBuffer::Ref Shader::ImportLoader::Load(lua_State *L, const char *name) {
	String path(CStr("@r:/Source/Shaders/"));
	path += name;
		
	file::MMapping::Ref mm = App::Get()->engine->sys->files->MapFile(path.c_str, ZTools);
	if (!mm)
		return lua::SrcBuffer::Ref();

	return lua::SrcBuffer::Ref(new lua::FileSrcBuffer(name, mm));
};

Shader::Shader(
	const char *name,
	const r::Material &m
) : m_name(name), m_skinMode(m.skinMode), m_genReflect(false), m_precisionMode(kPrecision_Low) {
	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
		if (m.TCGen(i) == r::Material::kTCGen_EnvMap) {
			m_genReflect = true;
			break;
		}
	}

	for (int i = 0; i < r::kMaxTextures; ++i)
		m_samplerPrecision[i] = kPrecision_Low;
}

Shader::Ref Shader::Load(
	Engine &e, 
	const char *name,
	const r::Material &mat
) {
	String path(CStr("@r:/Source/Shaders/"));
	path += name;
	path += ".shader";

	file::MMapping::Ref mm = e.sys->files->MapFile(path.c_str, ZTools);
	if (!mm) {
		COut(C_ErrMsgBox) << "tools::shader_utils::Shader::Load(): failed to load '" << name << "'" << std::endl;
		return Ref();
	}

	Shader::Ref m(new (ZTools) Shader(name, mat));
	lua::State::Ref L = InitLuaM(e, m.get());

	if (luaL_loadbuffer(
		L->L,
		(const char *)mm->data.get(),
		mm->size,
		name
	) != 0) {
		COut(C_ErrMsgBox) << "tools::shader_utils::Shader::Load(): " << lua_tostring(L->L, -1) << std::endl;
		return Shader::Ref();
	}

	if (lua_pcall(L->L, 0, 0, 0)) {
		COut(C_ErrMsgBox) << "tools::shader_utils::Shader::Load(): " << lua_tostring(L->L, -1) << std::endl;
		return Shader::Ref();
	}

	int numPasses = 0;
	for (int i = 0; i < r::Shader::kNumPasses; ++i) {
		if (m->PassOutputs((r::Shader::Pass)i)) {
			++numPasses;
			break;
		}
	}

	if (!numPasses)
		m.reset();

	return m;
}

int Shader::lua_MNode(lua_State *L) { 
	// in: 2 strings
    // out: io table
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	Shader *self = (Shader*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	RAD_VERIFY(self);

	lua_getfield(L, LUA_REGISTRYINDEX, ENG);
	Engine *e = (Engine*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	RAD_VERIFY(e);

	if (lua_type(L, -1) != LUA_TSTRING ||
		lua_type(L, -2) != LUA_TSTRING) {
		luaL_error(L, "Invalid arguments for Node(), expected (string, string), (Function %s, File %s, Line %d).",
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	String stype(lua_tostring(L, 1));
	String sname(lua_tostring(L, 2));
	lua_pop(L, 2);

	Node::Ref node = self->LoadNode(*e, L, stype.c_str);
	node->m_name = sname;
	self->m_instances.push_back(node);
	
	// table
	lua_createtable(L, 0, 3);
	lua_createtable(L, 0, 3);
	new (lua_newuserdata(L, sizeof(Node::Ref))) Node::Ref(node);
	luaL_getmetatable(L, GCT);
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "udata");
	lua_pushinteger(L, kNodeType_Frag);
	lua_setfield(L, -2, "type");
	lua_pushstring(L, "r");
	lua_setfield(L, -2, "r");
	lua_setfield(L, -2, META);
	// inputs
	lua_createtable(L, 0, (int)node->inputs->size());
	lua_setfield(L, -2, "In");

	// outputs
	lua_createtable(L, 0, (int)node->outputs->size());
		
	for (Node::OutputList::const_iterator it = node->outputs->begin(); it != node->outputs->end(); ++it) {
		lua_createtable(L, 0, 1);
		// copy inputs table
		lua_getfield(L, -3, "In");
		lua_setfield(L, -2, "In");
		lua_createtable(L, 0, 3);
		new (lua_newuserdata(L, sizeof(Node::Ref))) Node::Ref(node);
		luaL_getmetatable(L, GCT);
		lua_setmetatable(L, -2);
		lua_setfield(L, -2, "udata");
		lua_pushinteger(L, kNodeType_Frag);
		lua_setfield(L, -2, "type");
		lua_pushstring(L, (*it)->name.get());
		lua_setfield(L, -2, "r");
		lua_setfield(L, -2, META);
		lua_setfield(L, -2, (*it)->name.get());
	}

	lua_setfield(L, -2, "Out");

	return 1;
}

int Shader::lua_Compile(lua_State *L) { 
	// in: table
    // out: none
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	Shader *self = (Shader*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	RAD_VERIFY(self);

	lua::Variant::Map map;
	lua::ParseVariantTable(L, map, true);

	self->ParseShaderPass(L, "Default", r::Shader::kPass_Default, map);
	self->BuildInputMappings(L, r::Shader::kPass_Default);

	for (int i = 0; i < r::kMaxLights; ++i) {
		char sz[64];
		sprintf(sz, "Diffuse%i", i+1);
		self->ParseShaderPass(L, sz, (r::Shader::Pass)(r::Shader::kPass_Diffuse1+i), map);
		self->BuildInputMappings(L, (r::Shader::Pass)(r::Shader::kPass_Diffuse1+i));

		sprintf(sz, "DiffuseSpecular%i", i+1);
		self->ParseShaderPass(L, sz, (r::Shader::Pass)(r::Shader::kPass_DiffuseSpecular1+i), map);
		self->BuildInputMappings(L, (r::Shader::Pass)(r::Shader::kPass_DiffuseSpecular1+i));
	}

	self->ParseShaderPass(L, "Fullbright", r::Shader::kPass_Fullbright, map);
	self->BuildInputMappings(L, r::Shader::kPass_Fullbright);

	self->ParseShaderPass(L, "Preview", r::Shader::kPass_Preview, map);
	if (!self->Exists(r::Shader::kPass_Preview))
		self->ParseShaderPass(L, "Default", r::Shader::kPass_Preview, map);
	self->BuildInputMappings(L, r::Shader::kPass_Preview);

	return 0;
}

int Shader::lua_SetPrecisionMode(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	Shader *self = (Shader*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	RAD_VERIFY(self);

	const char *sz = luaL_checkstring(L, 1);
	const String ksz(CStr(sz));

	if (ksz == "low") {
		self->m_precisionMode = kPrecision_Low;
	} else if (ksz == "medium") {
		self->m_precisionMode = kPrecision_Medium;
	} else if (ksz == "high") {
		self->m_precisionMode = kPrecision_High;
	} else {
		luaL_error(L, "ERROR: %s in not a valid precision mode.", sz);
	}

	return 0;
}

int Shader::lua_SetSamplerPrecision(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	Shader *self = (Shader*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	RAD_VERIFY(self);

	int idx = 1;
	int samplerIndex = -1;
	if (lua_gettop(L) >= 2) {
		int samplerIndex = luaL_checknumber(L, 1);
		if ((samplerIndex < 0 || samplerIndex > r::kMaxTextures)) {
			luaL_error(L, "ERROR: %d is not a valid sampler index.", samplerIndex);
		}
		++idx;
	}

	const char *sz = luaL_checkstring(L, idx);
	const String ksz(CStr(sz));

	Precision mode = kPrecision_High;
	if (ksz == "low") {
		mode = kPrecision_Low;
	} else if (ksz == "medium") {
		mode = kPrecision_Medium;
	} else if (ksz != "high") {
		luaL_error(L, "ERROR: %s in not a valid precision mode.", sz);
	}

	if (idx == 2) {
		self->m_samplerPrecision[samplerIndex] = mode;
	} else {
		for (int i = 0; i < r::kMaxTextures; ++i)
			self->m_samplerPrecision[i] = mode;
	}

	return 0;
}

int Shader::lua_MColor(lua_State *L) {
	return lua_MSource(L, kMaterialSource_Color);
}

int Shader::lua_MSpecularColor(lua_State *L) {
	return lua_MSource(L, kMaterialSource_SpecularColor);
}

int Shader::lua_MTexture(lua_State *L) {
	return lua_MSource(L, kMaterialSource_Texture);
}

int Shader::lua_MTexCoord(lua_State *L) {
	return lua_MSource(L, kMaterialSource_TexCoord);
}

int Shader::lua_MFramebuffer(lua_State *L) {
	lua_pushinteger(L, 0);
	return lua_MSource(L, kMaterialSource_Framebuffer);
}

int Shader::lua_MVertex(lua_State *L) {
	return lua_MSource(L, kMaterialSource_Vertex);
}

int Shader::lua_MNormal(lua_State *L) {
	return lua_MSource(L, kMaterialSource_Normal);
}

int Shader::lua_MLightPos(lua_State *L) {
	return lua_MSource(L, kMaterialSource_LightPos);
}

int Shader::lua_MLightVec(lua_State *L) {
	return lua_MSource(L, kMaterialSource_LightVec);
}

int Shader::lua_MLightHalfVec(lua_State *L) {
	return lua_MSource(L, kMaterialSource_LightHalfVec);
}

int Shader::lua_MLightVertex(lua_State *L) {
	return lua_MSource(L, kMaterialSource_LightVertex);
}

int Shader::lua_MLightDiffuseColor(lua_State *L) {
	return lua_MSource(L, kMaterialSource_LightDiffuseColor);
}

int Shader::lua_MLightSpecularColor(lua_State *L) {
	return lua_MSource(L, kMaterialSource_LightSpecularColor);
}

int Shader::lua_MLightTanVec(lua_State *L) {
	return lua_MSource(L, kMaterialSource_LightTanVec);
}

int Shader::lua_MLightTanHalfVec(lua_State *L) {
	return lua_MSource(L, kMaterialSource_LightTanHalfVec);
}

int Shader::lua_MVertexColor(lua_State *L) {
	return lua_MSource(L, kMaterialSource_VertexColor);
}

int Shader::lua_MSpecularExponent(lua_State *L) {
	return lua_MSource(L, kMaterialSource_SpecularExponent);
}

int Shader::lua_MSource(lua_State *L, MaterialSource source) {
	if (lua_type(L, -1) != LUA_TNUMBER) {
		luaL_error(L, "Invalid arguments for MSource(int), (Function %s, File %s, Line %d).",
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	int idx = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_createtable(L, 0, 1);
	lua_createtable(L, 0, 3);
	lua_pushinteger(L, kNodeType_MaterialSource);
	lua_setfield(L, -2, "type");
	lua_pushinteger(L, source);
	lua_setfield(L, -2, "source");
	lua_pushinteger(L, idx);
	lua_setfield(L, -2, "idx");
	lua_setfield(L, -2, META);

	return 1;
}

int Shader::lua_gcNode(lua_State *L) {
	RAD_ASSERT(lua_type(L, -1) == LUA_TUSERDATA);
	Node::Ref *ref = (Node::Ref*)lua_touserdata(L, -1);
	ref->~shared_ptr();
	return 0;
}

lua::State::Ref Shader::InitLuaM(Engine &e, Shader *m) {
	lua::State::Ref state(new (ZTools) lua::State("ShaderFile"));
	lua_State *L = state->L;
	
	luaL_Reg r[] = {
		{ "Node", lua_MNode },
		{ "Compile", lua_Compile },
		{ "SetPrecisionMode", lua_SetPrecisionMode },
		{ "SetSamplerPrecision", lua_SetSamplerPrecision },
		{ "MColor", lua_MColor },
		{ "MSpecularColor", lua_MSpecularColor },
		{ "MTexture", lua_MTexture },
		{ "MFramebuffer", lua_MFramebuffer },
		{ "MTexCoord", lua_MTexCoord },
		{ "MVertex", lua_MVertex },
		{ "MNormal", lua_MNormal },
		{ "MLightPos", lua_MLightPos },
		{ "MLightVec", lua_MLightVec },
		{ "MLightHalfVec", lua_MLightHalfVec },
		{ "MLightVertex", lua_MLightVertex },
		{ "MLightDiffuseColor", lua_MLightDiffuseColor },
		{ "MLightSpecularColor", lua_MLightSpecularColor },
		{ "MLightTanVec", lua_MLightTanVec },
		{ "MLightTanHalfVec", lua_MLightTanHalfVec },
		{ "MVertexColor", lua_MVertexColor },
		{ "MSpecularExponent", lua_MSpecularExponent },
		{ 0, 0 }
	};

	lua::RegisterGlobals(L, 0, r);
	lua_pushlightuserdata(L, m);
	lua_setfield(L, LUA_REGISTRYINDEX, SELF);
	lua_pushlightuserdata(L, &e);
	lua_setfield(L, LUA_REGISTRYINDEX, ENG);

	if (luaL_newmetatable(L, GCT)) {
		lua_pushcfunction(L, lua_gcNode);
		lua_setfield(L, -2, "__gc");
	}

	lua_pop(L, 1);

	lua::EnableModuleImport(L, m->m_impLoader);

	return state;
}

lua::State::Ref Shader::InitLuaN(Node *n) {
	lua::State::Ref state(new (ZTools) lua::State("ShaderNode"));
	RegisterBasicTypes(state->L);
	
	luaL_Reg r[] = {
		{ "Node", lua_NNode },
		{ 0, 0 }
	};

	lua::RegisterGlobals(state->L, 0, r);
	lua_pushlightuserdata(state->L, n);
	lua_setfield(state->L, LUA_REGISTRYINDEX, SELF);

	return state;
}

int Shader::lua_NNode(lua_State *L) { 
	// input: table
	// output: none
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	Node *node = (Node*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	RAD_VERIFY(node);

	lua::Variant::Map map;
	lua::ParseVariantTable(L, map, true);

	// parse outputs

	lua::Variant::Map::const_iterator it = map.find(String("Outputs"));
	if (it == map.end()) {
		luaL_error(L, "Node type %s has no outputs!, (Function %s, File %s, Line %d).",
			node->type.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	const lua::Variant::Map *table = static_cast<const lua::Variant::Map*>(it->second);
	if (!table) {
		luaL_error(L, "Node type %s invalid outputs table!, (Function %s, File %s, Line %d).",
			node->type.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	for (lua::Variant::Map::const_iterator outIt = table->begin(); outIt != table->end(); ++outIt) {
		const lua::Variant::Map *m = static_cast<const lua::Variant::Map*>(outIt->second);
		if (!m) {
			luaL_error(L, "Node type %s invalid output field %s!, (Function %s, File %s, Line %d).",
				node->type.get(),
				outIt->first.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		Output::Ref output(new Output());
		output->m_name = outIt->first;
		
		ParseConnection(L, node, *m, *output.get());

		node->m_outputs.push_back(output);
	}

	// parse inputs

	it = map.find(String("Inputs"));
	if (it != map.end()) {
		const lua::Variant::Map *table = static_cast<const lua::Variant::Map*>(it->second);
		if (!table) {
			luaL_error(L, "Node type %s invalid inputs table!, (Function %s, File %s, Line %d).",
				node->type.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		for (lua::Variant::Map::const_iterator inIt = table->begin(); inIt != table->end(); ++inIt) {
			const lua::Variant::Map *m = static_cast<const lua::Variant::Map*>(inIt->second);
			if (!m) {
				luaL_error(L, "Node type %s invalid input field %s!, (Function %s, File %s, Line %d).",
					node->type.get(),
					inIt->first.c_str.get(),
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			Input::Ref input(new Input());
			input->m_name = inIt->first;

			ParseConnection(L, node, *m, *input.get());

			node->m_inputs.push_back(input);
		}
	}

	// alias?
	it = map.find(String("Alias"));
	if (it != map.end()) {
		const String *s = static_cast<const String*>(it->second);
		if (!s) {
			luaL_error(L, "Node type %s invalid alias (alias must be a string)!, (Function %s, File %s, Line %d).",
				node->type.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}
		node->m_alias = *s;
	}

	return 0;
}

void Shader::ParseConnection(lua_State *L, Node *node, const lua::Variant::Map &map, Connection &c) {
	c.m_node = node->shared_from_this();

	lua::Variant::Map::const_iterator it = map.find(String("t"));
	if (it == map.end()) {
		luaL_error(L, "Node type %s output %s missing type!, (Function %s, File %s, Line %d).",
			node->type.get(),
			c.name.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}
	
	const lua_Number *type = static_cast<const lua_Number*>(it->second);
	if (!type) {
		luaL_error(L, "Node type %s output %s invalid type!, (Function %s, File %s, Line %d).",
			node->type.get(),
			c.name.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	it = map.find(String("r"));
	if (it == map.end()) {
		luaL_error(L, "Node type %s output %s missing register!, (Function %s, File %s, Line %d).",
			node->type.get(),
			c.name.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	const lua_Number *r = static_cast<const lua_Number*>(it->second);
	if (!r) {
		luaL_error(L, "Node type %s output %s register invalid!, (Function %s, File %s, Line %d).",
			node->type.get(),
			c.name.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	const String *s = 0;
	it = map.find(String("s"));
	if (it != map.end()) {
		s = static_cast<const String*>(it->second);
		if (!s) {
			luaL_error(L, "Node type %s output %s semantic must be a string!, (Function %s, File %s, Line %d).",
				node->type.get(),
				c.name.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}
	}

	c.m_type = (BasicType)(int)*type;
	c.m_r = (int)*r;
	if (s)
		c.m_semantic = *s;
}

Shader::Node::Ref Shader::LoadNode(Engine &e, lua_State *L, const char *type) {
	{
		NodeMap::iterator it = m_types.find(String(type));
		if (it != m_types.end())
			return it->second->Clone();
	}

	String path(CStr("@r:/Source/Shaders/Nodes/"));
	path += type;
	path += ".node";

	file::MMapping::Ref mm = e.sys->files->MapFile(path.c_str, ZTools);
	if (!mm) {
		luaL_error(L, "tools::shader_utils::Shader::LoadNode(): Error loading %s, (Function %s, File %s, Line %d).",
			type,
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	Node::Ref n(new (ZTools) Node());
	n->m_type = type;
	lua::State::Ref S = InitLuaN(n.get());

	if (luaL_loadbuffer(
		S->L,
		(const char *)mm->data.get(),
		mm->size,
		type
	) != 0) {
		luaL_error(L, "tools::shader_utils::Shader::LoadNode(): Error loading %s (%s), (Function %s, File %s, Line %d).",
			type,
			lua_tostring(S->L, -1),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	if (lua_pcall(S->L, 0, 0, 0)) {
		luaL_error(L, "tools::shader_utils::Shader::LoadNode(): Error loading %s (%s), (Function %s, File %s, Line %d).",
			type,
			lua_tostring(S->L, -1),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	m_types[String(type)] = n;
	return n->Clone();
}

void Shader::ParseShaderPass(
	lua_State *L,
	const char *name, 
	r::Shader::Pass pass,
	const lua::Variant::Map &map
) {
	lua::Variant::Map::const_iterator it = map.find(String(name));
	if (it == map.end())
		return;
	const lua::Variant &v = it->second;
	const lua::Variant::Map *m = static_cast<const lua::Variant::Map*>(v);
	if (!m) {
		luaL_error(L, "Expected table! (Function %s, File %s, Line %d).",
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	Root root;
	NodeMap stack;

	{ // Color output
		it = m->find(String("color"));
		if (it != m->end()) {
			const lua::Variant::Map *rootM = static_cast<const lua::Variant::Map*>(it->second);
			if (!rootM) {
				luaL_error(L, "Expected table! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			root.out[kShaderOutput_Color].input.reset(new Input());
			root.out[kShaderOutput_Color].node = ParseNodes(L, root.out[kShaderOutput_Color].input, *rootM, stack);
			if (!root.out[kShaderOutput_Color].node && 
				root.out[kShaderOutput_Color].input->msource == kMaterialSource_Node) {
				luaL_error(L, "Invalid material color output. (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			RAD_ASSERT(stack.empty());

			if (root.out[kShaderOutput_Color].node)
				GatherUsage(root.out[kShaderOutput_Color].node, m_usage[pass]);
			else
				AddUsage(root.out[kShaderOutput_Color].input, m_usage[pass]);
		}
	}

	{ // Seperate alpha output
		it = m->find(String("alpha"));
		if (it != m->end()) {
			const lua::Variant::Map *rootM = static_cast<const lua::Variant::Map*>(it->second);
			if (!rootM) {
				luaL_error(L, "Expected table! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			root.out[kShaderOutput_Alpha].input.reset(new Input());
			root.out[kShaderOutput_Alpha].node = ParseNodes(L, root.out[kShaderOutput_Alpha].input, *rootM, stack);
			if (!root.out[kShaderOutput_Alpha].node && root.out[kShaderOutput_Alpha].input->msource == kMaterialSource_Node)
			{
				luaL_error(L, "Invalid material alpha output. (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			RAD_ASSERT(stack.empty());

			if (root.out[kShaderOutput_Alpha].node)
				GatherUsage(root.out[kShaderOutput_Alpha].node, m_usage[pass]);
			else
				AddUsage(root.out[kShaderOutput_Alpha].input, m_usage[pass]);
		}
	}

	{ // Depth output
		it = m->find(String("depth"));
		if (it != m->end()) {
			const lua::Variant::Map *rootM = static_cast<const lua::Variant::Map*>(it->second);
			if (!rootM) {
				luaL_error(L, "Expected table! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			root.out[kShaderOutput_Depth].input.reset(new Input());
			root.out[kShaderOutput_Depth].node = ParseNodes(L, root.out[kShaderOutput_Depth].input, *rootM, stack);
			if (!root.out[kShaderOutput_Depth].node && root.out[kShaderOutput_Depth].input->msource == kMaterialSource_Node)
			{
				luaL_error(L, "Invalid material depth output. (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			RAD_ASSERT(stack.empty());

			if (root.out[kShaderOutput_Depth].node)
				GatherUsage(root.out[kShaderOutput_Depth].node, m_usage[pass]);
			else
				AddUsage(root.out[kShaderOutput_Depth].input, m_usage[pass]);
		}
	}

	m_nodes[pass] = root;
}

Shader::Node::Ref Shader::ParseNodes(
	lua_State *L,
	const Input::Ref &input,
	const lua::Variant::Map &map,
	NodeMap &stack
) {
	RAD_ASSERT(input);
	lua::Variant::Map::const_iterator it = map.find(String(META));
	if (it == map.end()) {
		luaL_error(L, "Expected META table! (Function %s, File %s, Line %d).",
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	Node::Ref node;

	// extract node from our own metatable
	{
		const lua::Variant::Map *meta = static_cast<const lua::Variant::Map*>(it->second);
		if (!meta) {
			luaL_error(L, "META field is not a table! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		it = meta->find(String("type"));
		if (it == meta->end()) {
			luaL_error(L, "Missing type field on META table! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const lua_Number *type = static_cast<const lua_Number*>(it->second);
		if (!type) {
			luaL_error(L, "Type field on META table is not int! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		if (*type == kNodeType_MaterialSource) { 
			// node type is material source, create the input node.
			it = meta->find(String("source"));
			if (it == meta->end()) {
				luaL_error(L, "Missing source field on META table! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}
			
			const lua_Number *source = static_cast<const lua_Number*>(it->second);
			if (!source) {
				luaL_error(L, "Source field on META table is not int! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			it = meta->find(String("idx"));
			if (it == meta->end()) {
				luaL_error(L, "Missing idx field on META table! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			const lua_Number *idx = static_cast<const lua_Number*>(it->second);
			if (!idx) {
				luaL_error(L, "Idx field on META table is not int! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			input->msource = (MaterialSource)(int)*source;
			input->msourceIdx = (int)*idx;
			return Node::Ref();
		}

		it = meta->find(String("udata"));
		if (it == meta->end()) {
			luaL_error(L, "Missing udata field on META table! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const Node::Ref **pref = (const Node::Ref**)static_cast<void*const*>(it->second);
		if (!pref) {
			luaL_error(L, "udata field on META table is null! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		node = *(*pref);

		it = meta->find(String("r"));
		if (it == meta->end()) {
			luaL_error(L, "Missing register field on META table! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const String *outputName = static_cast<const String*>(it->second);
		if (!outputName) {
			luaL_error(L, "Register field on META table not a string! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		Output::Ref output;

		if (*outputName == "r")  {
			// return register, may not be named "r" so use index -1
			output = node->FindOutputRegister(-1);
			if (!output)
			{
				luaL_error(L, "Node %s does not have a return register! (Function %s, File %s, Line %d).",
					node->name.get(),
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}
		} else {
			output = node->FindOutputName(outputName->c_str);
			if (!output)
			{
				luaL_error(L, "Node %s does not have output named %s! (Function %s, File %s, Line %d).",
					node->name.get(),
					outputName->c_str.get(),
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}
		}

		input->source = output;
	}

	if (stack.find(node->m_name) != stack.end()) {
		luaL_error(L, "Recursive material node! (Function %s, File %s, Line %d).",
			node->name.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	NodeMap::iterator stackPos = stack.insert(NodeMap::value_type(node->m_name, node)).first;
	
	// follow inputs...

	it = map.find(String("In"));
	if (it == map.end()) {
		luaL_error(L, "Node '%s' is missing inputs table! (Function %s, File %s, Line %d).",
			node->name.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	const lua::Variant::Map *inputs = static_cast<const lua::Variant::Map*>(it->second);
	if (!inputs) {
		luaL_error(L, "Node '%s' input field is not a table! (Function %s, File %s, Line %d).",
			node->name.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	for (lua::Variant::Map::const_iterator mit = inputs->begin(); mit != inputs->end(); ++mit) {
		const lua::Variant::Map *input = static_cast<const lua::Variant::Map*>(mit->second);
		if (!input) {
			luaL_error(L, "Invalid node input (%s, %s)! (Function %s, File %s, Line %d).",
				node->name.get(),
				mit->first.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		Input::Ref in = node->FindInputName(mit->first.c_str);

		if (!in) {
			luaL_error(L, "Node %s does not have input named %s! (Function %s, File %s, Line %d).",
				node->name.get(),
				mit->first.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		ParseNodes(L, in, *input, stack);
	}

	stack.erase(stackPos);

	return node;
}

bool Shader::Exists(r::Shader::Pass pass) const {
	for (int i = 0; i < kNumShaderOutputs; ++i) {
		if (m_nodes[pass].out[i].node||m_nodes[pass].out[i].input)
			return true;
	}
	return false;
}

int Shader::PassOutputs(r::Shader::Pass pass) const {
	int flags = 0;
	for (int i = 0; i < kNumShaderOutputs; ++i) {
		if (m_nodes[pass].out[i].node||m_nodes[pass].out[i].input)
			flags += (1<<i);
	}
	return flags;
}

void Shader::GatherUsage(
	const Node::Ref &root,
	Usage &usage
) {
	for (Node::InputList::const_iterator it = root->inputs->begin(); it != root->inputs->end(); ++it) {
		const Input::Ref &input = *it;
		if (input->msource != kMaterialSource_Node)
			AddUsage(input, usage);
		else if(input->source.get())
			GatherUsage(input->source->node, usage);
	}
}

void Shader::AddUsage(
	const Input::Ref &input,
	Usage &usage
) {
	RAD_ASSERT(input->msource != kMaterialSource_Node);
	usage.s[input->msource.get()].insert(input->msourceIdx.get());
}

void Shader::BuildInputMappings(lua_State *L, r::Shader::Pass pass) {
	int numTexs = 0;
	int numAttrs = 0;

	Usage &usage = m_usage[pass];
	r::MaterialInputMappings &mapping = m_mapping[pass];
	memset(&mapping, 0, sizeof(mapping));

	BuildTextureSourceMapping(
		L, 
		numTexs, 
		r::kMaterialTextureSource_Texture, 
		usage.s[kMaterialSource_Texture], 
		mapping
	);

	for (;numTexs < r::kMaxTextures; ++numTexs) {
		mapping.textures[numTexs][0] = mapping.textures[numTexs][1] = r::kInvalidMapping;
	}

	// do this so we don't pollute MaterialSourceUsage()
	IntSet vertexUsage = usage.s[kMaterialSource_Vertex];

	// force mapping to contain vertex stream 0, otherwise no vertex
	// data will be passed to shader.
	vertexUsage.insert(0);
	
	BuildAttributeSourceMapping(
		L, 
		numAttrs, 
		r::kMaterialGeometrySource_Vertices, 
		vertexUsage, 
		mapping
	);

	// NOTE: Normal, Tangent are needed if shader accessed the LightDir field.
	IntSet normalUsage = usage.s[kMaterialSource_Normal];
	IntSet tangentUsage = usage.s[kMaterialSource_Tangent];

	if (!usage.s[kMaterialSource_LightTanVec].empty() ||
		!usage.s[kMaterialSource_LightTanHalfVec].empty()) {
		// add references to Normal/Tangent.
		normalUsage.insert(0);
		tangentUsage.insert(0);
		// NOTE: bitangent is computed by vertex shader if LightTangentVec/LightTangentHalfVec is accessed.
	}

	if (m_genReflect) {
		// normals are required for environment mapping.
		normalUsage.insert(0);
	}

	BuildAttributeSourceMapping(
		L, 
		numAttrs, 
		r::kMaterialGeometrySource_Normals, 
		normalUsage, 
		mapping
	);

	BuildAttributeSourceMapping(
		L, 
		numAttrs, 
		r::kMaterialGeometrySource_Tangents, 
		tangentUsage, 
		mapping
	);

	BuildAttributeSourceMapping(
		L, 
		numAttrs, 
		r::kMaterialGeometrySource_VertexColor, 
		usage.s[kMaterialSource_VertexColor], 
		mapping
	);

	if ((pass != r::Shader::kPass_Preview) && (m_skinMode == r::Material::kSkinMode_Sprite)) {
		// sprite skin needs special vertex-shader args
		usage.s[kMaterialSource_SpriteSkin].insert(0);
		BuildAttributeSourceMapping(
			L, 
			numAttrs, 
			r::kMaterialGeometrySource_SpriteSkin, 
			usage.s[kMaterialSource_SpriteSkin], 
			mapping
		);
	}

	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i)
		mapping.tcMods[i] = r::kInvalidMapping;

	for (;numAttrs < r::kMaxAttribArrays; ++numAttrs) {
		mapping.attributes[numAttrs][0] = mapping.attributes[numAttrs][1] = r::kInvalidMapping;
	}
}

bool Shader::BuildInputMappings(
	const r::Material &material, 
	r::Shader::Pass pass,
	r::MaterialInputMappings &mapping,
	TexCoordMapping &tcMapping
) const {
	mapping = m_mapping[pass];
	tcMapping = BuildTexCoordMapping(material, pass);

	if (tcMapping.size() >= r::kMaterialTextureSource_MaxIndices) {
		COut(C_Error) << "ERROR: tcMapping register overflow!" << std::endl;
		return false;
	}

	int ofs = 0;
	for (TexCoordMapping::const_iterator it = tcMapping.begin(); it != tcMapping.end(); ++it) {
		int i;
		for (i = 0; i < ofs; ++i) {
			if (mapping.tcMods[i] == (*it).second)
				break;
		}

		if (i == ofs) {
			mapping.tcMods[ofs++] = (*it).second;
		}
	}

	for (; ofs < r::kMaxTextures; ++ofs)
		mapping.tcMods[ofs] = r::kInvalidMapping;

	// pack texcoord inputs in canonical order

	int numAttribs;

	for (numAttribs = 0; numAttribs < r::kMaxAttribArrays; ++numAttribs) {
		if (mapping.attributes[numAttribs][0] == r::kInvalidMapping)
			break;
	}

	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
		for (int k = 0; k < r::kMaterialTextureSource_MaxIndices; ++k) {
			if (mapping.tcMods[k] == r::kInvalidMapping)
				break;
			int uvIndex = material.TCUVIndex((int)mapping.tcMods[k]);
			if (uvIndex == i) { // UV channel is referenced in material.
				if (numAttribs >= r::kMaxAttribArrays) {
					COut(C_Error) << "ERROR: tcInput register overflow!" << std::endl;
					return false;
				}
				mapping.attributes[numAttribs][0] = r::kMaterialGeometrySource_TexCoords;
				mapping.attributes[numAttribs][1] = (U8)uvIndex;
				++mapping.numMGSources[r::kMaterialGeometrySource_TexCoords];
				++numAttribs;
				break;
			}
		}
	}
	
	return true;
}

Shader::TexCoordMapping Shader::BuildTexCoordMapping(const r::Material &m, r::Shader::Pass pass) const {
	// The vertex shader will generate a set of texture coordinate inputs for the pixel shader
	// which we are generating. Only a single Identity/Reflect TCGen should be created by the vertex shader
	// (the first one encountered). All other cases will be uniquely created.

	const IntSet &usage = m_usage[pass].s[kMaterialSource_TexCoord];
	int ofs = 0;
	int index = 0;
	boost::array<std::pair<int, int>, r::kMaxAttribArrays> identity;
	boost::array<std::pair<int, int>, r::kMaxAttribArrays> envMap;

	for (int i = 0; i < r::kMaxAttribArrays; ++i) {
		identity[i].first = -1;
		identity[i].second = -1;
		envMap[i].first = -1;
		envMap[i].second = -1;
	}

	TexCoordMapping mapping;
	mapping.resize(usage.size(), std::pair<int, int>(-1, -1));

	for (IntSet::const_iterator it = usage.begin(); it != usage.end(); ++it, ++ofs) {
		int i;
		for (i = 0; i < r::Material::kNumTCMods; ++i) {
			if (m.Wave(*it, i, r::Material::kTexCoord_S).type != WaveAnim::T_Identity) {
				break;
			}
		}

		if (i == r::Material::kNumTCMods) {
			int uvIndex = m.TCUVIndex(*it);

			// identity
			if (m.TCGen(*it) == r::Material::kTCGen_Vertex) {
				if (identity[uvIndex].first == -1) {
					identity[uvIndex].first = index++;
					identity[uvIndex].second = *it;
				}
				mapping[ofs] = identity[uvIndex];
			} else {
				if (envMap[uvIndex].first == -1) {
					envMap[uvIndex].first = index++;
					envMap[uvIndex].second = *it;
				}
				mapping[ofs] = envMap[uvIndex];
			}			
		} else {
			mapping[ofs].first = index++;
			mapping[ofs].second = *it;
		}
	}

	return mapping;
}

void Shader::BuildTextureSourceMapping(
	lua_State *L,
	int &num, 
	r::MaterialTextureSource source,
	const IntSet &usage,
	r::MaterialInputMappings &mapping
) {
	for (IntSet::const_iterator it = usage.begin(); it != usage.end(); ++it) {
		if (num >= r::kMaxTextures) {
			luaL_error(L, "Texture limits exceeded! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		mapping.textures[num][0] = (U8)source;
		mapping.textures[num][1] = (U8)*it;
		++mapping.numMTSources[source];
		++num;
	}
}

void Shader::BuildAttributeSourceMapping(
	lua_State *L,
	int &num,
	r::MaterialGeometrySource source,
	const IntSet &usage,
	r::MaterialInputMappings &mapping
) {
	for (IntSet::const_iterator it = usage.begin(); it != usage.end(); ++it) {
		if (num >= r::kMaxAttribArrays) {
			luaL_error(L, "Attribute limits exceeded! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		mapping.attributes[num][0] = (U8)source;
		mapping.attributes[num][1] = (U8)*it;
		++mapping.numMGSources[source];
		++num;
	}
}

bool Shader::EmitFunctions(Engine &engine, std::ostream &out) const {
	for (NodeMap::const_iterator it = m_types.begin(); it != m_types.end(); ++it) {
		if (!it->second->m_alias.empty)
			continue;

		String path(CStr("@r:/Source/Shaders/Nodes/"));
		path += it->first;
		path += ".code";
		out << "// " << path << "\r\n";
		if (!Inject(engine, path.c_str, out)) {
			return false;
		}
		out << "\r\n";
	}
	return true;
}

bool Shader::EmitShader(
	const char *fnName, 
	r::Shader::Pass pass, 
	const r::Material &m,
	std::ostream &out
) const {
	out << "M " << fnName << "(VOID_GLOBALS)\r\n{\r\n";
	out << "\tM R;\r\n";

	TexCoordMapping tcMapping = BuildTexCoordMapping(m, pass);

	// color output.
	int outflags = PassOutputs(pass);

	bool alpha = m_nodes[pass].out[kShaderOutput_Alpha].node || m_nodes[pass].out[kShaderOutput_Alpha].input;

	int ofs = 0;
	VarMap map;

	if (outflags & kShaderOutputFlag_Color) {
		if (!EmitOutput(
			pass,
			tcMapping,
			kShaderOutput_Color,
			alpha ? "color.xyz" : "color",
			map,
			ofs,
			out
		)) {
			return false;
		}
	} else {
		out << "\tR.color = PRECISION_COLOR_TYPE(1.0, 1.0, 1.0, 1.0);\r\n";
	}

	if (alpha) {
		if (!EmitOutput(
			pass,
			tcMapping,
			kShaderOutput_Alpha,
			"color.w",
			map,
			ofs,
			out
		)) {
			return false;
		}
	}
	
	if (outflags & kShaderOutputFlag_Depth) {
		if (!EmitOutput(
			pass,
			tcMapping,
			kShaderOutput_Depth,
			"depth",
			map,
			ofs,
			out
		)) {
			return false;
		}
	}

	out << "\treturn R;\r\n}\r\n";
	return true;
}

bool Shader::EmitOutput(
	r::Shader::Pass pass,
	const TexCoordMapping &tcMapping,
	ShaderOutput index,
	const char *semantic,
	VarMap &vars,
	int &varOfs,
	std::ostream &out
) const {
	const Node::Ref &n = m_nodes[pass].out[index].node;
	
	if (n) {
		Output::Ref r = m_nodes[pass].out[index].input->source;
		if (!EmitShaderNode(pass, tcMapping, n, vars, varOfs, out))
			return false;

		char sz[64];
		VarMap::const_iterator var = vars.find(r.get());
		RAD_ASSERT(var != vars.end());

		string::sprintf(sz, "r%.3i", var->second);
		out << "\tR." << semantic << " = " << sz << ";\r\n";
	} else { 
		// global material input
		MaterialSource msource = m_nodes[pass].out[index].input->msource;
		int msourceIdx = m_nodes[pass].out[index].input->msourceIdx;
		RAD_ASSERT(msource != kMaterialSource_Node);
		char sz[64];
		RAD_VERIFY(szMaterialInput(sz, pass, tcMapping, msource, msourceIdx));
		out << "\tR." << semantic << " = " << sz << ";\r\n";
	}

	return true;
}

bool Shader::EmitShaderNode(
	r::Shader::Pass pass,
	const TexCoordMapping &tcMapping,
	const Node::Ref &node,
	VarMap &vars,
	int &varOfs,
	std::ostream &out
) const {
	// Done already?
	if (vars.find(node.get()) != vars.end())
		return true;

	// Gather inputs...
	for (Node::InputList::const_iterator it = node->inputs->begin(); it != node->inputs->end(); ++it) {
		const Input::Ref &input = *it;
		if (input->msource != kMaterialSource_Node)
			continue;
		if (!input->source.get()) {
			COut(C_Error) << "Node is missing input (" << node->type.get() << ":\"" << node->name.get() << "\", In." << 
				input->name.get() << ")!" << std::endl;
			return false;
		}
		if (!EmitShaderNode(pass, tcMapping, input->source->node, vars, varOfs, out))
			return false;
	}

	// make outputs / ordered list of arguments.
	int r = -1;
	char sz[64];

	typedef zone_map<int, String, ZToolsT>::type IntToStringMap;
	IntToStringMap map;

	for (Node::OutputList::const_iterator it = node->outputs->begin(); it != node->outputs->end(); ++it) {
		const Output::Ref &output = *it;
		vars.insert(VarMap::value_type(output.get(), varOfs));
		if (output->r == -1) {
			r = varOfs;
		} else {
			string::sprintf(sz, "r%.3i", varOfs);
			map.insert(IntToStringMap::value_type(output->r, String(sz)));
		}
		string::sprintf(sz, "%s r%.3i", s_types[output->type], varOfs++);
		out << "\t" << sz << ";\r\n";
	}

	// gather arguments

	for (Node::InputList::const_iterator it = node->inputs->begin(); it != node->inputs->end(); ++it) {
		const Input::Ref &input = *it;
		if (input->msource == kMaterialSource_Node) {
			VarMap::const_iterator var = vars.find(input->source.get().get());
			RAD_ASSERT(var != vars.end()); // this shouldn't be possible.
			string::sprintf(sz, "r%.3i", var->second);
		} else { 
			// global material input.
			RAD_VERIFY(szMaterialInput(sz, pass, tcMapping, input->msource, input->msourceIdx));
		}

		map.insert(IntToStringMap::value_type(input->r, String(sz)));
	}

	// generate function call

	if (r != -1) { 
		// return...
		string::sprintf(sz, "r%.3i", r);
		out << "\t" << sz << " = ";
	} else {
		out << "\t";
	}

	if (node->m_alias.empty) {
		out << node->type.get() << "(";
	} else {
		out << node->alias.get() << "(";
	}

	for (IntToStringMap::const_iterator it = map.begin(); it != map.end(); ++it) {
		if (it != map.begin())
			out << ", ";
		out << it->second;
	}

	if (node->m_alias.empty) {
		out << " P_GLOBALS);\r\n";
	} else {
		out << ");\r\n";
	}

	vars.insert(VarMap::value_type(node.get(), -1));
	return true;
}

bool Shader::szMaterialInput(
	char *sz, 
	r::Shader::Pass pass,
	const TexCoordMapping &tcMapping,
	MaterialSource source, 
	int index
) const {
	
	switch (source) {
	case kMaterialSource_Texture:
	case kMaterialSource_Framebuffer:
		string::sprintf(
			sz, 
			"UNIFORM(t%d)", 
			TextureUsageIndex(
				pass, 
				source, 
				index
			)
		);
		return true;
	case kMaterialSource_Color:
		strcpy(sz, "U_color");
		return true;
	case kMaterialSource_SpecularColor:
		strcpy(sz, "U_scolor");
		return true;
	case kMaterialSource_SpecularExponent:
		strcpy(sz, "U_scolor.w");
		return true;
	case kMaterialSource_LightDiffuseColor:
		string::sprintf(
			sz,
			"UNIFORM(light%i_diffuseColor)",
			index
		);
		return true;
	case kMaterialSource_LightSpecularColor:
		string::sprintf(
			sz,
			"UNIFORM(light%i_specularColor)",
			index
		);
		return true;
	case kMaterialSource_LightPos:
		string::sprintf(
			sz,
			"UNIFORM(light%i_pos)",
			index
		);
		return true;
	case kMaterialSource_LightVec:
		string::sprintf(
			sz,
			"normalize(IN(light%i_vec))",
			index
		);
		return true;
	case kMaterialSource_LightHalfVec:
		string::sprintf(
			sz,
			"normalize(IN(light%i_halfvec))",
			index
		);
		return true;
	case kMaterialSource_LightVertex:
		string::sprintf(
			sz,
			"IN(light%i_vpos)",
			index
		);
		return true;
	case kMaterialSource_Vertex:
		strcpy(sz, "IN(position)");
		return true;
	case kMaterialSource_Normal:
		string::sprintf(
			sz,
			"normalize(IN(nm%d))",
			AttribUsageIndex(
				pass,
				source, 
				index
			)
		);
		return true;
	case kMaterialSource_Tangent:
		string::sprintf(
			sz,
			"IN(tan%d)",
			AttribUsageIndex(
				pass,
				source, 
				index
			)
		);
		return true;
	case kMaterialSource_Bitangent:
		string::sprintf(
			sz,
			"IN(bitan%d)",
			AttribUsageIndex(
				pass,
				source, 
				index
			)
		);
		return true;
	case kMaterialSource_LightTanVec:
		 // see Shaders/Nodes/Common.c comments
		if (index < 3) {
			string::sprintf(
				sz,
				"normalize(IN(light%d_tanvec))",
				index
			);
		} else {
			string::sprintf(
				sz,
				"normalize(IN(light_%d_tanvec))",
				index
			);
		}
		return true;
	case kMaterialSource_LightTanHalfVec:
		string::sprintf(
			sz,
			"normalize(IN(light%d_tanhalfvec))",
			index
		);
		return true;
	case kMaterialSource_TexCoord:
		string::sprintf(
			sz,
			"IN(TEXCOORD%d)",
			AttribUsageIndex(
				pass,
				source, 
				index
			)
		);
		return true;
	case kMaterialSource_VertexColor:
		strcpy(sz, "IN(vertexColor)");
		return true;
	default:
		break;
	}

	return false;
}

int Shader::TextureUsageIndex(r::Shader::Pass pass, MaterialSource source, int index) const {
	RAD_ASSERT(source >= kMaterialSource_Texture);
	RAD_ASSERT(source <= kMaterialSource_Framebuffer);

	int ofs = -1;
	const IntSet &usage = m_usage[pass].s[source];
	for (IntSet::const_iterator it = usage.begin(); it != usage.end(); ++it) {
		++ofs;
		if (*it == index)
			break;
	}

	RAD_VERIFY(ofs != -1);

	switch (source)
	{
	case kMaterialSource_Texture:
		return ofs;
	case kMaterialSource_Framebuffer: // pack framebuffer after textures
		return (int)m_usage[pass].s[kMaterialSource_Texture].size() + ofs;
	default:
		break;
	}

	RAD_FAIL("Invalid texture source");
	return -1;
}

int Shader::AttribUsageIndex(r::Shader::Pass pass, MaterialSource source, int index) const {
	RAD_ASSERT(source >= kMaterialSource_Vertex);
	RAD_ASSERT(source <= kMaterialSource_TexCoord);

	int ofs = -1;
	const IntSet &usage = m_usage[pass].s[source];
	for (IntSet::const_iterator it = usage.begin(); it != usage.end(); ++it) {
		++ofs;
		if (*it == index)
			break;
	}

	RAD_VERIFY(ofs != -1);

	return ofs;
}

int Shader::TexCoordUsageIndex(
	r::Shader::Pass pass,
	const TexCoordMapping &tcMapping,
	MaterialSource source, 
	int index
) const {
	int idx = AttribUsageIndex(pass, source, index);
	return tcMapping[idx].first;
}

//////////////////////////////////////////////////////////////////////////////////////////

Shader::Input::Ref Shader::Node::FindInputName(const char *name) {
	for (InputList::const_iterator it = m_inputs.begin(); it != m_inputs.end(); ++it) {
		const Input::Ref &input = *it;
		if (!string::cmp(input->name.get(), name))
			return input;
	}

	return Input::Ref();
}

Shader::Input::Ref Shader::Node::FindInputSemantic(const char *semantic) {
	for (InputList::const_iterator it = m_inputs.begin(); it != m_inputs.end(); ++it) {
		const Input::Ref &input = *it;
		if (!string::cmp(input->semantic.get(), semantic))
			return input;
	}

	return Input::Ref();
}

Shader::Input::Ref Shader::Node::FindInputRegister(int r) {
	for (InputList::const_iterator it = m_inputs.begin(); it != m_inputs.end(); ++it) {
		const Input::Ref &input = *it;
		if (input->r == r)
			return input;
	}

	return Input::Ref();
}

Shader::Output::Ref Shader::Node::FindOutputName(const char *name) {
	for (OutputList::const_iterator it = m_outputs.begin(); it != m_outputs.end(); ++it) {
		const Output::Ref &output = *it;
		if (!string::cmp(output->name.get(), name))
			return output;
	}

	return Output::Ref();
}

Shader::Output::Ref Shader::Node::FindOutputSemantic(const char *semantic) {
	for (OutputList::const_iterator it = m_outputs.begin(); it != m_outputs.end(); ++it) {
		const Output::Ref &output = *it;
		if (!string::cmp(output->semantic.get(), semantic))
			return output;
	}

	return Output::Ref();
}

Shader::Output::Ref Shader::Node::FindOutputRegister(int r) {
	for (OutputList::const_iterator it = m_outputs.begin(); it != m_outputs.end(); ++it) {
		const Output::Ref &output = *it;
		if (output->r == r)
			return output;
	}

	return Output::Ref();
}

Shader::Node::Ref Shader::Node::Clone() const {
	Node::Ref clone(new (ZTools) Node());
	clone->m_name = m_name;
	clone->m_type = m_type;
	clone->m_alias = m_alias;

	for (InputList::const_iterator it = m_inputs.begin(); it != m_inputs.end(); ++it) {
		const Input::Ref &src = *it;
		Input::Ref dst(new (ZTools) Input());
		dst->m_name = src->m_name;
		dst->m_type = src->m_type;
		dst->m_semantic = src->m_semantic;
		dst->m_r = src->m_r;
		dst->m_node = clone;
		clone->m_inputs.push_back(dst);
	}

	for (OutputList::const_iterator it = m_outputs.begin(); it != m_outputs.end(); ++it) {
		const Output::Ref &src = *it;
		Output::Ref dst(new (ZTools) Output());
		dst->m_name = src->m_name;
		dst->m_type = src->m_type;
		dst->m_semantic = src->m_semantic;
		dst->m_r = src->m_r;
		dst->m_node = clone;
		clone->m_outputs.push_back(dst);
	}

	return clone;
}

//////////////////////////////////////////////////////////////////////////////////////////

Shader::Ref ShaderCache::Load(
	Engine &engine, 
	const char *name,
	const r::Material &mat
) {
	RAD_ASSERT(name);
	String sname(name);
	ShaderMap::const_iterator it = m_shaders[mat.skinMode].find(sname);
	if (it != m_shaders[mat.skinMode].end())
		return it->second;

	Shader::Ref m = Shader::Load(engine, name, mat);
	if (m)
		m_shaders[mat.skinMode][sname] = m;

	return m;
}

} // shader_utils
} // tool

