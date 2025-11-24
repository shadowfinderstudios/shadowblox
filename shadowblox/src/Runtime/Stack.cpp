// SPDX-License-Identifier: MPL-2.0
/*******************************************************************************
 * shadowblox - https://git.seki.pw/Fumohouse/shadowblox
 *
 * Copyright 2025-present ksk.
 * Copyright 2025-present shadowblox contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See COPYRIGHT.txt for more details.
 ******************************************************************************/

#include "Sbx/Runtime/Stack.hpp"

#include <cmath>
#include <cstdint>
#include <string>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Base.hpp"

/* BASIC TYPES */

namespace SBX {

#define BASIC_STACK_OP_IMPL(type, name, opName, isName)                                                                \
	void LuauStackOp<type>::Push(lua_State *L, const type &value) { lua_push##opName(L, value); }                      \
	type LuauStackOp<type>::Get(lua_State *L, int index) { return static_cast<type>(lua_to##opName(L, index)); }       \
	bool LuauStackOp<type>::Is(lua_State *L, int index) { return lua_is##isName(L, index); }                           \
	type LuauStackOp<type>::Check(lua_State *L, int index) { return static_cast<type>(luaL_check##opName(L, index)); } \
                                                                                                                       \
	const std::string LuauStackOp<type>::NAME = name;

BASIC_STACK_OP_IMPL(bool, "bool", boolean, boolean);
BASIC_STACK_OP_IMPL(int, "int", integer, number);
BASIC_STACK_OP_IMPL(float, "float", number, number);
BASIC_STACK_OP_IMPL(double, "double", number, number);
BASIC_STACK_OP_IMPL(int8_t, "int", number, number);
BASIC_STACK_OP_IMPL(uint8_t, "int", unsigned, number);
BASIC_STACK_OP_IMPL(int16_t, "int", number, number);
BASIC_STACK_OP_IMPL(uint16_t, "int", unsigned, number);
BASIC_STACK_OP_IMPL(uint32_t, "int", unsigned, number);

/* 64-BIT INTEGER */

#define INT64_MT_NAME "Luau.Int64"
#define INT64_DOUBLE_LIMIT 9007199254740992 // 2^53; see https://stackoverflow.com/a/1848762

void LuauStackOp<int64_t>::InitMetatable(lua_State *L) {
	luaL_newmetatable(L, INT64_MT_NAME);

	lua_pushstring(L, "Int64");
	lua_setfield(L, -2, "__type");

#define INT64_OP_FUNC(exp, name, ret)                                                                    \
	lua_pushcfunction(                                                                                   \
			L, [](lua_State *L) {                                                                        \
				double d1 = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : LuauStackOp<int64_t>::Check(L, 1); \
				double d2 = lua_isnumber(L, 2) ? lua_tonumber(L, 2) : LuauStackOp<int64_t>::Check(L, 2); \
				LuauStackOp<ret>::Push(L, exp);                                                          \
				return 1;                                                                                \
			},                                                                                           \
			INT64_MT_NAME "." name);                                                                     \
                                                                                                         \
	lua_setfield(L, -2, name);

	INT64_OP_FUNC(d1 + d2, "__add", double)
	INT64_OP_FUNC(d1 - d2, "__sub", double)
	INT64_OP_FUNC(d1 * d2, "__mul", double)
	INT64_OP_FUNC(d1 / d2, "__div", double)
	INT64_OP_FUNC(fmod(d1, d2), "__mod", double)
	INT64_OP_FUNC(pow(d1, d2), "__pow", double)

	INT64_OP_FUNC(d1 == d2, "__eq", bool)
	INT64_OP_FUNC(d1 < d2, "__lt", bool)

	// Special: negate
	lua_pushcfunction(
			L, [](lua_State *L) {
				LuauStackOp<int64_t>::Push(L, -LuauStackOp<int64_t>::Check(L, 1));
				return 1;
			},
			INT64_MT_NAME ".__unm");

	lua_setfield(L, -2, "__unm");

	// Special: stringify
	lua_pushcfunction(
			L, [](lua_State *L) {
				LuauStackOp<std::string>::Push(L, std::to_string(LuauStackOp<int64_t>::Check(L, 1)));
				return 1;
			},
			INT64_MT_NAME ".__tostring");

	lua_setfield(L, -2, "__tostring");

	lua_setreadonly(L, -1, true);
	lua_setuserdatametatable(L, Int64Udata);
}

void LuauStackOp<int64_t>::PushI64(lua_State *L, const int64_t &value) {
	int64_t *udata = reinterpret_cast<int64_t *>(lua_newuserdatataggedwithmetatable(L, sizeof(int64_t), Int64Udata));
	*udata = value;
}

void LuauStackOp<int64_t>::Push(lua_State *L, const int64_t &value) {
	if (value >= -INT64_DOUBLE_LIMIT && value <= INT64_DOUBLE_LIMIT) {
		lua_pushnumber(L, value);
	} else {
		LuauStackOp<int64_t>::PushI64(L, value);
	}
}

int64_t LuauStackOp<int64_t>::Get(lua_State *L, int index) {
	if (lua_isnumber(L, index)) {
		return lua_tonumber(L, index);
	}

	int64_t *udata = reinterpret_cast<int64_t *>(lua_touserdatatagged(L, index, Int64Udata));
	if (udata) {
		return *udata;
	}

	return 0;
}

bool LuauStackOp<int64_t>::Is(lua_State *L, int index) {
	return lua_isnumber(L, index) || lua_touserdatatagged(L, index, Int64Udata);
}

int64_t LuauStackOp<int64_t>::Check(lua_State *L, int index) {
	int64_t *udata = reinterpret_cast<int64_t *>(lua_touserdatatagged(L, index, Int64Udata));
	if (udata) {
		return *udata;
	}

	return luaL_checknumber(L, index);
}

const std::string LuauStackOp<int64_t>::NAME = "int64";

/* STRING */

void LuauStackOp<std::string>::Push(lua_State *L, const std::string &value) {
	lua_pushstring(L, value.c_str());
}

std::string LuauStackOp<std::string>::Get(lua_State *L, int index) {
	return lua_tostring(L, index);
}

bool LuauStackOp<std::string>::Is(lua_State *L, int index) {
	return lua_isstring(L, index);
}

std::string LuauStackOp<std::string>::Check(lua_State *L, int index) {
	return luaL_checkstring(L, index);
}

const std::string LuauStackOp<std::string>::NAME = "string";

void LuauStackOp<const char *>::Push(lua_State *L, const char *value) { lua_pushstring(L, value); }
const char *LuauStackOp<const char *>::Get(lua_State *L, int index) { return lua_tostring(L, index); }
bool LuauStackOp<const char *>::Is(lua_State *L, int index) { return lua_isstring(L, index); }
const char *LuauStackOp<const char *>::Check(lua_State *L, int index) { return luaL_checkstring(L, index); }

const std::string LuauStackOp<const char *>::NAME = "string";

} //namespace SBX
