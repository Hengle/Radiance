// CGShader.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#if defined(RAD_OPT_TOOLS)

#include "CGShader.h"
#include "CGUtils.h"
#include "../../Lua/LuaRuntime.h"
#include "../../COut.h"
#include "../../Engine.h"
#include <Runtime/File.h>

#define SELF "@t"
#define ENG "@e"
#define META "@m"
#define GCT "@g"

namespace cg {
namespace {

enum NodeType
{
	NT_Frag,
	NT_MSource
};

const char *s_ord[Shader::OrdMax] =
{
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

void RegisterOrdinals(lua_State *L)
{
	for (int i = 0; i < Shader::OrdMax; ++i)
	{
		lua_pushnumber(L, (lua_Number)i);
		lua_setglobal(L, s_ord[i]);
	}
}

} // namespace

Shader::Ref Shader::Load(Engine &e, const char *name)
{
	String path(CStr("Shaders/"));
	path += name;
	path += ".sh";

	file::MMapping::Ref mm = e.sys->files->MapFile(path.c_str, ZTools);
	if (!mm)
	{
		COut(C_ErrMsgBox) << "cg::Shader::Load(): failed to load '" << name << "'" << std::endl;
		return Ref();
	}

	Shader::Ref m(new (ZTools) Shader(name));
	lua::State::Ref L = InitLuaM(e, m.get());

	if (luaL_loadbuffer(
		L->L,
		(const char *)mm->data.get(),
		mm->size,
		name
	) != 0)
	{
		COut(C_ErrMsgBox) << "cg::Shader::Load(): " << lua_tostring(L->L, -1) << std::endl;
		return Shader::Ref();
	}

	if (lua_pcall(L->L, 0, 0, 0))
	{
		COut(C_ErrMsgBox) << "cg::Shader::Load(): " << lua_tostring(L->L, -1) << std::endl;
		return Shader::Ref();
	}

	if (m->ChannelOutputs(C_Default) == 0)
	{ // never called Compile()
		m.reset();
	}

	return m;
}

int Shader::lua_MNode(lua_State *L)
{ // in: 2 strings
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
		lua_type(L, -2) != LUA_TSTRING)
	{
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
	lua_pushinteger(L, NT_Frag);
	lua_setfield(L, -2, "type");
	lua_pushstring(L, "r");
	lua_setfield(L, -2, "r");
	lua_setfield(L, -2, META);
	// inputs
	lua_createtable(L, 0, (int)node->inputs->size());
	lua_setfield(L, -2, "In");

	// outputs
	lua_createtable(L, 0, (int)node->outputs->size());
		
	for (Node::OutputList::const_iterator it = node->outputs->begin(); it != node->outputs->end(); ++it)
	{
		lua_createtable(L, 0, 1);
		// copy inputs table
		lua_getfield(L, -3, "In");
		lua_setfield(L, -2, "In");
		lua_createtable(L, 0, 3);
		new (lua_newuserdata(L, sizeof(Node::Ref))) Node::Ref(node);
		luaL_getmetatable(L, GCT);
		lua_setmetatable(L, -2);
		lua_setfield(L, -2, "udata");
		lua_pushinteger(L, NT_Frag);
		lua_setfield(L, -2, "type");
		lua_pushstring(L, (*it)->name.get());
		lua_setfield(L, -2, "r");
		lua_setfield(L, -2, META);
		lua_setfield(L, -2, (*it)->name.get());
	}

	lua_setfield(L, -2, "Out");

	return 1;
}

int Shader::lua_Compile(lua_State *L)
{ // in: table
  // out: none
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	Shader *self = (Shader*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	RAD_VERIFY(self);

	lua::Variant::Map map;
	lua::ParseVariantTable(L, map, true);

	self->ParseChannel(L, "Default", C_Default, map);
	self->BuildInputMappings(L, C_Default);

	return 0;
}

int Shader::lua_MColor(lua_State *L)
{
	return lua_MSource(L, MS_Color);
}

int Shader::lua_MTexture(lua_State *L)
{
	return lua_MSource(L, MS_Texture);
}

int Shader::lua_MTexCoord(lua_State *L)
{
	return lua_MSource(L, MS_TexCoord);
}

int Shader::lua_MFramebuffer(lua_State *L)
{
	lua_pushinteger(L, 0);
	return lua_MSource(L, MS_Framebuffer);
}

int Shader::lua_MSource(lua_State *L, MSource source)
{
	if (lua_type(L, -1) != LUA_TNUMBER)
	{
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
	lua_pushinteger(L, NT_MSource);
	lua_setfield(L, -2, "type");
	lua_pushinteger(L, source);
	lua_setfield(L, -2, "source");
	lua_pushinteger(L, idx);
	lua_setfield(L, -2, "idx");
	lua_setfield(L, -2, META);

	return 1;
}

int Shader::lua_gcNode(lua_State *L)
{
	RAD_ASSERT(lua_type(L, -1) == LUA_TUSERDATA);
	Node::Ref *ref = (Node::Ref*)lua_touserdata(L, -1);
	ref->~shared_ptr();
	return 0;
}

lua::State::Ref Shader::InitLuaM(Engine &e, Shader *m)
{
	lua::State::Ref state(new (ZTools) lua::State("ShadersM"));
	lua_State *L = state->L;

	luaL_Reg r[] =
	{
		{ "Node", lua_MNode },
		{ "Compile", lua_Compile },
		{ "MColor", lua_MColor },
		{ "MTexture", lua_MTexture },
		{ "MTexCoord", lua_MTexCoord },
		{ "MFramebuffer", lua_MFramebuffer },
		{ 0, 0 }
	};

	lua::RegisterGlobals(L, 0, r);
	lua_pushlightuserdata(L, m);
	lua_setfield(L, LUA_REGISTRYINDEX, SELF);
	lua_pushlightuserdata(L, &e);
	lua_setfield(L, LUA_REGISTRYINDEX, ENG);

	if (luaL_newmetatable(L, GCT))
	{
		lua_pushcfunction(L, lua_gcNode);
		lua_setfield(L, -2, "__gc");
	}

	lua_pop(L, 1);

	return state;
}

lua::State::Ref Shader::InitLuaN(Node *n)
{
	lua::State::Ref state(new (ZTools) lua::State("ShadersN"));
	RegisterOrdinals(state->L);
	
	luaL_Reg r[] =
	{
		{ "Node", lua_NNode },
		{ 0, 0 }
	};

	lua::RegisterGlobals(state->L, 0, r);
	lua_pushlightuserdata(state->L, n);
	lua_setfield(state->L, LUA_REGISTRYINDEX, SELF);

	return state;
}

int Shader::lua_NNode(lua_State *L)
{ // input: table
  // output: none
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	Node *node = (Node*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	RAD_VERIFY(node);

	lua::Variant::Map map;
	lua::ParseVariantTable(L, map, true);

	// parse outputs

	lua::Variant::Map::const_iterator it = map.find(String("Outputs"));
	if (it == map.end())
	{
		luaL_error(L, "Node type %s has no outputs!, (Function %s, File %s, Line %d).",
			node->type.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	const lua::Variant::Map *table = static_cast<const lua::Variant::Map*>(it->second);
	if (!table)
	{
		luaL_error(L, "Node type %s invalid outputs table!, (Function %s, File %s, Line %d).",
			node->type.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	for (lua::Variant::Map::const_iterator outIt = table->begin(); outIt != table->end(); ++outIt)
	{
		const lua::Variant::Map *m = static_cast<const lua::Variant::Map*>(outIt->second);
		if (!m)
		{
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
	if (it != map.end())
	{
		const lua::Variant::Map *table = static_cast<const lua::Variant::Map*>(it->second);
		if (!table)
		{
			luaL_error(L, "Node type %s invalid inputs table!, (Function %s, File %s, Line %d).",
				node->type.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		for (lua::Variant::Map::const_iterator inIt = table->begin(); inIt != table->end(); ++inIt)
		{
			const lua::Variant::Map *m = static_cast<const lua::Variant::Map*>(inIt->second);
			if (!m)
			{
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
	if (it != map.end())
	{
		const String *s = static_cast<const String*>(it->second);
		if (!s)
		{
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

void Shader::ParseConnection(lua_State *L, Node *node, const lua::Variant::Map &map, Connection &c)
{
	c.m_node = node->shared_from_this();

	lua::Variant::Map::const_iterator it = map.find(String("t"));
	if (it == map.end())
	{
		luaL_error(L, "Node type %s output %s missing type!, (Function %s, File %s, Line %d).",
			node->type.get(),
			c.name.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}
	
	const lua_Number *type = static_cast<const lua_Number*>(it->second);
	if (!type)
	{
		luaL_error(L, "Node type %s output %s invalid type!, (Function %s, File %s, Line %d).",
			node->type.get(),
			c.name.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	it = map.find(String("r"));
	if (it == map.end())
	{
		luaL_error(L, "Node type %s output %s missing register!, (Function %s, File %s, Line %d).",
			node->type.get(),
			c.name.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	const lua_Number *r = static_cast<const lua_Number*>(it->second);
	if (!r)
	{
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
	if (it != map.end())
	{
		s = static_cast<const String*>(it->second);
		if (!s)
		{
			luaL_error(L, "Node type %s output %s semantic must be a string!, (Function %s, File %s, Line %d).",
				node->type.get(),
				c.name.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}
	}

	c.m_type = (Ordinal)(int)*type;
	c.m_r = (int)*r;
	if (s)
		c.m_semantic = *s;
}

Shader::Node::Ref Shader::LoadNode(Engine &e, lua_State *L, const char *type)
{
	{
		NodeMap::iterator it = m_types.find(String(type));
		if (it != m_types.end())
			return it->second->Clone();
	}

	String path(CStr("Shaders/Nodes/"));
	path += type;
	path += ".n";

	file::MMapping::Ref mm = e.sys->files->MapFile(path.c_str, ZTools);
	if (!mm)
	{
		luaL_error(L, "cg::Shader::LoadNode(): Error loading %s, (Function %s, File %s, Line %d).",
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
	) != 0)
	{
		luaL_error(L, "cg::Shader::LoadNode(): Error loading %s (%s), (Function %s, File %s, Line %d).",
			type,
			lua_tostring(S->L, -1),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	if (lua_pcall(S->L, 0, 0, 0))
	{
		luaL_error(L, "cg::Shader::LoadNode(): Error loading %s (%s), (Function %s, File %s, Line %d).",
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

void Shader::ParseChannel(
	lua_State *L,
	const char *name, 
	Channel channel,
	const lua::Variant::Map &map
)
{
	lua::Variant::Map::const_iterator it = map.find(String(name));
	if (it == map.end())
		return;
	const lua::Variant &v = it->second;
	const lua::Variant::Map *m = static_cast<const lua::Variant::Map*>(v);
	if (!m)
	{
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
		if (it != m->end())
		{
			const lua::Variant::Map *rootM = static_cast<const lua::Variant::Map*>(it->second);
			if (!rootM)
			{
				luaL_error(L, "Expected table! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			root.out[O_Color].input.reset(new Input());
			root.out[O_Color].node = ParseNodes(L, root.out[O_Color].input, *rootM, stack);
			if (!root.out[O_Color].node && root.out[O_Color].input->msource == MS_Node)
			{
				luaL_error(L, "Invalid material color output. (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			RAD_ASSERT(stack.empty());

			if (root.out[O_Color].node)
				GatherUsage(root.out[O_Color].node, m_usage[channel]);
			else
				AddUsage(root.out[O_Color].input, m_usage[channel]);
		}
	}

	{ // Color output
		it = m->find(String("alpha"));
		if (it != m->end())
		{
			const lua::Variant::Map *rootM = static_cast<const lua::Variant::Map*>(it->second);
			if (!rootM)
			{
				luaL_error(L, "Expected table! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			root.out[O_Alpha].input.reset(new Input());
			root.out[O_Alpha].node = ParseNodes(L, root.out[O_Alpha].input, *rootM, stack);
			if (!root.out[O_Alpha].node && root.out[O_Alpha].input->msource == MS_Node)
			{
				luaL_error(L, "Invalid material alpha output. (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			RAD_ASSERT(stack.empty());

			if (root.out[O_Alpha].node)
				GatherUsage(root.out[O_Alpha].node, m_usage[channel]);
			else
				AddUsage(root.out[O_Alpha].input, m_usage[channel]);
		}
	}

	{ // Depth output
		it = m->find(String("depth"));
		if (it != m->end())
		{
			const lua::Variant::Map *rootM = static_cast<const lua::Variant::Map*>(it->second);
			if (!rootM)
			{
				luaL_error(L, "Expected table! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			root.out[O_Depth].input.reset(new Input());
			root.out[O_Depth].node = ParseNodes(L, root.out[O_Depth].input, *rootM, stack);
			if (!root.out[O_Depth].node && root.out[O_Depth].input->msource == MS_Node)
			{
				luaL_error(L, "Invalid material depth output. (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			RAD_ASSERT(stack.empty());

			if (root.out[O_Depth].node)
				GatherUsage(root.out[O_Depth].node, m_usage[channel]);
			else
				AddUsage(root.out[O_Depth].input, m_usage[channel]);
		}
	}

	m_nodes[channel] = root;
}

Shader::Node::Ref Shader::ParseNodes(
	lua_State *L,
	const Input::Ref &input,
	const lua::Variant::Map &map,
	NodeMap &stack
)
{
	RAD_ASSERT(input);
	lua::Variant::Map::const_iterator it = map.find(String(META));
	if (it == map.end())
	{
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
		if (!meta)
		{
			luaL_error(L, "META field is not a table! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		it = meta->find(String("type"));
		if (it == meta->end())
		{
			luaL_error(L, "Missing type field on META table! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const lua_Number *type = static_cast<const lua_Number*>(it->second);
		if (!type)
		{
			luaL_error(L, "Type field on META table is not int! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		if (*type == NT_MSource)
		{ // node type is material source, create the input node.
			it = meta->find(String("source"));
			if (it == meta->end())
			{
				luaL_error(L, "Missing source field on META table! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}
			
			const lua_Number *source = static_cast<const lua_Number*>(it->second);
			if (!source)
			{
				luaL_error(L, "Source field on META table is not int! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			it = meta->find(String("idx"));
			if (it == meta->end())
			{
				luaL_error(L, "Missing idx field on META table! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			const lua_Number *idx = static_cast<const lua_Number*>(it->second);
			if (!idx)
			{
				luaL_error(L, "Idx field on META table is not int! (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			input->msource = (MSource)(int)*source;
			input->msourceIdx = (int)*idx;
			return Node::Ref();
		}

		it = meta->find(String("udata"));
		if (it == meta->end())
		{
			luaL_error(L, "Missing udata field on META table! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const Node::Ref **pref = (const Node::Ref**)static_cast<void*const*>(it->second);
		if (!pref)
		{
			luaL_error(L, "udata field on META table is null! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		node = *(*pref);

		it = meta->find(String("r"));
		if (it == meta->end())
		{
			luaL_error(L, "Missing register field on META table! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const String *outputName = static_cast<const String*>(it->second);
		if (!outputName)
		{
			luaL_error(L, "Register field on META table not a string! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		Output::Ref output;

		if (*outputName == "r") // return register, may not be named "r" so use index -1
		{
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
		}
		else
		{
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

	if (stack.find(node->m_name) != stack.end())
	{
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
	if (it == map.end())
	{
		luaL_error(L, "Node '%s' is missing inputs table! (Function %s, File %s, Line %d).",
			node->name.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	const lua::Variant::Map *inputs = static_cast<const lua::Variant::Map*>(it->second);
	if (!inputs)
	{
		luaL_error(L, "Node '%s' input field is not a table! (Function %s, File %s, Line %d).",
			node->name.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	for (lua::Variant::Map::const_iterator mit = inputs->begin(); mit != inputs->end(); ++mit)
	{
		const lua::Variant::Map *input = static_cast<const lua::Variant::Map*>(mit->second);
		if (!input)
		{
			luaL_error(L, "Invalid node input (%s, %s)! (Function %s, File %s, Line %d).",
				node->name.get(),
				mit->first.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		Input::Ref in = node->FindInputName(mit->first.c_str);

		if (!in)
		{
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

void Shader::GatherUsage(
	const Node::Ref &root,
	Usage &usage
)
{
	for (Node::InputList::const_iterator it = root->inputs->begin(); it != root->inputs->end(); ++it)
	{
		const Input::Ref &input = *it;
		if (input->msource != MS_Node)
			AddUsage(input, usage);
		else if(input->source.get())
			GatherUsage(input->source->node, usage);
	}
}

void Shader::AddUsage(
	const Input::Ref &input,
	Usage &usage
)
{
	RAD_ASSERT(input->msource != MS_Node);
	usage.s[input->msource.get()].insert(input->msourceIdx.get());
}

void Shader::BuildInputMappings(lua_State *L, Channel channel)
{
	int numTexs = 0;
	int numAttrs = 0;

	Usage &usage = m_usage[channel];
	r::GLState::MInputMappings &mapping = m_mapping[channel];
	memset(&mapping, 0, sizeof(mapping));

	// force vertex attribute.
	usage.s[MS_Vertex].insert(0);

	BuildTextureSourceMapping(L, numTexs, r::MTS_Texture, usage.s[MS_Texture], mapping);
	BuildTextureSourceMapping(L, numTexs, r::MTS_Framebuffer, usage.s[MS_Framebuffer], mapping);

	for (;numTexs < r::GLState::MaxTextures; ++numTexs)
	{
		mapping.textures[numTexs][0] = mapping.textures[numTexs][1] = r::InvalidMapping;
	}

	BuildAttributeSourceMapping(L, numAttrs, r::MGS_Vertices, usage.s[MS_Vertex], mapping);

	int firstTexCoord = numAttrs;

	BuildAttributeSourceMapping(L, numAttrs, r::MGS_TexCoords, usage.s[MS_TexCoord], mapping);

	for (int i = firstTexCoord; i < numAttrs; ++i) // record texture register assignments
		mapping.attributes[i][2] = (U8)(i-firstTexCoord);

	for (;numAttrs < r::GLState::MaxAttribArrays; ++numAttrs)
	{
		mapping.attributes[numAttrs][0] = mapping.attributes[numAttrs][1] = mapping.attributes[numAttrs][2] = r::InvalidMapping;
	}
}

void Shader::BuildTextureSourceMapping(
	lua_State *L,
	int &num, 
	r::MTSource source,
	const IntSet &usage,
	r::GLState::MInputMappings &mapping
)
{
	for (IntSet::const_iterator it = usage.begin(); it != usage.end(); ++it)
	{
		if (num > r::GLState::MaxTextures)
		{
			luaL_error(L, "Texture limits exceeded! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		mapping.textures[num][0] = (U8)source;
		mapping.textures[num][1] = (U8)*it;
		++mapping.numMTSources[source];
		++mapping.numTexs;
		++num;
	}
}

void Shader::BuildAttributeSourceMapping(
	lua_State *L,
	int &num,
	r::MGSource source,
	const IntSet &usage,
	r::GLState::MInputMappings &mapping
)
{
	for (IntSet::const_iterator it = usage.begin(); it != usage.end(); ++it)
	{
		if (num > (r::GLState::MaxAttribArrays-r::GLState::NumSkinArrays))
		{
			luaL_error(L, "Attribute limits exceeded! (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		mapping.attributes[num][0] = (U8)source;
		mapping.attributes[num][1] = (U8)*it;
		mapping.attributes[num][2] = (U8)*it;
		++mapping.numMGSources[source];
		++mapping.numAttrs;
		++num;
	}
}

bool Shader::EmitFunctions(Engine &engine, std::ostream &out) const
{
	for (NodeMap::const_iterator it = m_types.begin(); it != m_types.end(); ++it)
	{
		if (!it->second->m_alias.empty)
			continue;

		String path(CStr("Shaders/Nodes/"));
		path += it->first;
		path += ".f";
		out << "// " << path << "\r\n";
		if (!Inject(engine, path.c_str, out))
		{
			return false;
		}
		out << "\r\n";
	}
	return true;
}

bool Shader::EmitShader(const char *fnName, Channel channel, std::ostream &out) const
{
	out << "M " << fnName << "(VOID_GLOBALS)\r\n{\r\n";
	out << "\tM R;\r\n";

	// color output.
	int outflags = ChannelOutputs(channel);

	bool alpha = m_nodes[channel].out[O_Alpha].node || m_nodes[channel].out[O_Alpha].input;

	int ofs = 0;
	VarMap map;

	if (outflags & OF_Color)
	{
		if (!EmitOutput(
			channel,
			O_Color,
			alpha ? "color.xyz" : "color",
			map,
			ofs,
			out
		))
		{
			return false;
		}
	}
	else
	{
		out << "\tR.color = FLOAT4(1.0, 1.0, 1.0, 1.0);\r\n";
	}

	if (alpha)
	{
		if (!EmitOutput(
			channel,
			O_Alpha,
			"color.w",
			map,
			ofs,
			out
		))
		{
			return false;
		}
	}
	
	if (outflags & OF_Depth)
	{
		if (!EmitOutput(
			channel,
			O_Depth,
			"depth",
			map,
			ofs,
			out
		))
		{
			return false;
		}
	}

	out << "\treturn R;\r\n}\r\n";
	return true;
}

bool Shader::EmitOutput(
	Channel channel,
	OutputIndex index,
	const char *semantic,
	VarMap &vars,
	int &varOfs,
	std::ostream &out
) const
{
	const Node::Ref &n = m_nodes[channel].out[index].node;
	
	if (n)
	{
		Output::Ref r = m_nodes[channel].out[index].input->source;
		if (!EmitShaderNode(channel, n, vars, varOfs, out))
			return false;

		char sz[64];
		VarMap::const_iterator var = vars.find(r.get());
		RAD_ASSERT(var != vars.end());

		string::sprintf(sz, "r%.3i", var->second);
		out << "\tR." << semantic << " = " << sz << ";\r\n";
	}
	else
	{ // global material input
		MSource msource = m_nodes[channel].out[index].input->msource;
		int msourceIdx = m_nodes[channel].out[index].input->msourceIdx;
		RAD_ASSERT(msource != MS_Node);
		char sz[64];
		RAD_VERIFY(szMaterialInput(sz, channel, msource, msourceIdx));
		out << "\tR." << semantic << " = " << sz << ";\r\n";
	}

	return true;
}

bool Shader::EmitShaderNode(
	Channel channel,
	const Node::Ref &node,
	VarMap &vars,
	int &varOfs,
	std::ostream &out
) const
{
	// Done already?
	if (vars.find(node.get()) != vars.end())
		return true;

	// Gather inputs...
	for (Node::InputList::const_iterator it = node->inputs->begin(); it != node->inputs->end(); ++it)
	{
		const Input::Ref &input = *it;
		if (input->msource != MS_Node)
			continue;
		if (!input->source.get())
		{
			COut(C_Error) << "Node is missing input (" << node->type.get() << ":\"" << node->name.get() << "\", In." << 
				input->name.get() << ")!" << std::endl;
			return false;
		}
		if (!EmitShaderNode(channel, input->source->node, vars, varOfs, out))
			return false;
	}

	// make outputs / ordered list of arguments.
	int r = -1;
	char sz[64];

	typedef zone_map<int, String, ZToolsT>::type IntToStringMap;
	IntToStringMap map;

	for (Node::OutputList::const_iterator it = node->outputs->begin(); it != node->outputs->end(); ++it)
	{
		const Output::Ref &output = *it;
		vars.insert(VarMap::value_type(output.get(), varOfs));
		if (output->r == -1)
		{
			r = varOfs;
		}
		else
		{
			string::sprintf(sz, "r%.3i", varOfs);
			map.insert(IntToStringMap::value_type(output->r, String(sz)));
		}
		string::sprintf(sz, "%s r%.3i", s_ord[output->type], varOfs++);
		out << "\t" << sz << ";\r\n";
	}

	// gather arguments

	for (Node::InputList::const_iterator it = node->inputs->begin(); it != node->inputs->end(); ++it)
	{
		const Input::Ref &input = *it;
		if (input->msource == MS_Node)
		{
			VarMap::const_iterator var = vars.find(input->source.get().get());
			RAD_ASSERT(var != vars.end()); // this shouldn't be possible.
			string::sprintf(sz, "r%.3i", var->second);
		}
		else
		{ // global material input.
			RAD_VERIFY(szMaterialInput(sz, channel, input->msource, input->msourceIdx));
		}

		map.insert(IntToStringMap::value_type(input->r, String(sz)));
	}

	// generate function call

	if (r != -1)
	{ // return...
		string::sprintf(sz, "r%.3i", r);
		out << "\t" << sz << " = ";
	}
	else
	{
		out << "\t";
	}

	if (node->m_alias.empty)
	{
		out << node->type.get() << "(";
	}
	else
	{
		out << node->alias.get() << "(";
	}

	for (IntToStringMap::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		if (it != map.begin())
			out << ", ";
		out << it->second;
	}

	if (node->m_alias.empty)
	{
		out << " P_GLOBALS);\r\n";
	}
	else
	{
		out << ");\r\n";
	}

	vars.insert(VarMap::value_type(node.get(), -1));
	return true;
}

bool Shader::szMaterialInput(char *sz, Channel channel, MSource source, int index) const
{
	switch (source)
	{
	case MS_Texture:
	case MS_Framebuffer:
		string::sprintf(
			sz, 
			"UNIFORM(t%d)", 
			TextureUsageIndex(
				channel, 
				source, 
				index
			)
		);
		return true;
	case MS_TexCoord:
		string::sprintf(
			sz,
			"IN(tc%d)",
			AttribUsageIndex(
				channel,
				source, 
				index
			)
		);
		return true;
	case MS_Color:
		string::sprintf(
			sz,
			"DCOLOR%d",
			index
		);
		return true;
	}

	return false;
}

int Shader::TextureUsageIndex(Channel channel, MSource source, int index) const
{
	RAD_ASSERT(source == MS_Texture || source == MS_Framebuffer);
	int ofs = 0;
	const IntSet &usage = m_usage[channel].s[source];
	for (IntSet::const_iterator it = usage.begin(); it != usage.end(); ++it)
	{
		if (*it == index)
			break;
		++ofs;
	}

	switch (source)
	{
	case MS_Texture:
		return ofs;
	case MS_Framebuffer:
		return (int)m_usage[channel].s[MS_Texture].size() + ofs;
	}

	RAD_FAIL("Invalid texture source");
	return -1;
}

int Shader::AttribUsageIndex(Channel channel, MSource source, int index) const
{
	RAD_ASSERT(source == MS_TexCoord);
	int ofs = 0;
	const IntSet &usage = m_usage[channel].s[source];
	for (IntSet::const_iterator it = usage.begin(); it != usage.end(); ++it)
	{
		if (*it == index)
			break;
		++ofs;
	}

	switch (source)
	{
	case MS_TexCoord:
		return ofs;
	}

	RAD_FAIL("Invalid attrib source");
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////

Shader::Input::Ref Shader::Node::FindInputName(const char *name)
{
	for (InputList::const_iterator it = m_inputs.begin(); it != m_inputs.end(); ++it)
	{
		const Input::Ref &input = *it;
		if (!string::cmp(input->name.get(), name))
			return input;
	}

	return Input::Ref();
}

Shader::Input::Ref Shader::Node::FindInputSemantic(const char *semantic)
{
	for (InputList::const_iterator it = m_inputs.begin(); it != m_inputs.end(); ++it)
	{
		const Input::Ref &input = *it;
		if (!string::cmp(input->semantic.get(), semantic))
			return input;
	}

	return Input::Ref();
}

Shader::Input::Ref Shader::Node::FindInputRegister(int r)
{
	for (InputList::const_iterator it = m_inputs.begin(); it != m_inputs.end(); ++it)
	{
		const Input::Ref &input = *it;
		if (input->r == r)
			return input;
	}

	return Input::Ref();
}

Shader::Output::Ref Shader::Node::FindOutputName(const char *name)
{
	for (OutputList::const_iterator it = m_outputs.begin(); it != m_outputs.end(); ++it)
	{
		const Output::Ref &output = *it;
		if (!string::cmp(output->name.get(), name))
			return output;
	}

	return Output::Ref();
}

Shader::Output::Ref Shader::Node::FindOutputSemantic(const char *semantic)
{
	for (OutputList::const_iterator it = m_outputs.begin(); it != m_outputs.end(); ++it)
	{
		const Output::Ref &output = *it;
		if (!string::cmp(output->semantic.get(), semantic))
			return output;
	}

	return Output::Ref();
}

Shader::Output::Ref Shader::Node::FindOutputRegister(int r)
{
	for (OutputList::const_iterator it = m_outputs.begin(); it != m_outputs.end(); ++it)
	{
		const Output::Ref &output = *it;
		if (output->r == r)
			return output;
	}

	return Output::Ref();
}

Shader::Node::Ref Shader::Node::Clone() const
{
	Node::Ref clone(new (ZTools) Node());
	clone->m_name = m_name;
	clone->m_type = m_type;
	clone->m_alias = m_alias;

	for (InputList::const_iterator it = m_inputs.begin(); it != m_inputs.end(); ++it)
	{
		const Input::Ref &src = *it;
		Input::Ref dst(new (ZTools) Input());
		dst->m_name = src->m_name;
		dst->m_type = src->m_type;
		dst->m_semantic = src->m_semantic;
		dst->m_r = src->m_r;
		dst->m_node = clone;
		clone->m_inputs.push_back(dst);
	}

	for (OutputList::const_iterator it = m_outputs.begin(); it != m_outputs.end(); ++it)
	{
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

void ShaderCache::Discover(Engine &engine)
{
	file::FileSearch::Ref search = engine.sys->files->OpenSearch(
		"Shaders/*.sh",
		file::kSearchOption_Recursive,
		file::kFileOptions_None,
		file::kFileMask_Base
	);

	if (search)
	{
		String path;
		while (search->NextFile(path))
		{
			String name = file::SetFileExtension(path.c_str, 0);
			Load(engine, name.c_str);
		}
	}
}

} // cg

#endif // RAD_OPT_TOOLS
