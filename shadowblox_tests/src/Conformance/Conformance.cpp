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

#include "doctest.h"

#include <fstream>
#include <ios>
#include <sstream>
#include <string>

#include "lua.h"
#include "lualib.h"

#include "Sbx/DataTypes/Types.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"
#include "Utils.hpp"

using namespace SBX;

static int lua_gccollect(lua_State *L) {
	lua_gc(L, LUA_GCCOLLECT, 0);
	return 0;
}

static int lua_asserterror(lua_State *L) {
	luaL_checktype(L, 1, LUA_TFUNCTION);
	std::string expectedErr = luaL_optstring(L, 2, "");

	lua_pushvalue(L, 1);
	int status = lua_pcall(L, 0, 0, 0);
	if (status == LUA_OK || status == LUA_YIELD) {
		luaL_error(L, "assertion failed! function did not error");
	}

	if (!expectedErr.empty()) {
		std::string err = lua_tostring(L, -1);
		std::string errTrimmed = err.substr(err.find_first_of(' ') + 1);

		if (errTrimmed != expectedErr) {
			luaL_error(L, "assertion failed! error doesn't match: expected '%s', got '%s'.",
					expectedErr.c_str(),
					errTrimmed.c_str());
		}
	}

	return 0;
}

static const luaL_Reg TEST_LIB[] = {
	{ "gccollect", lua_gccollect },
	{ "asserterror", lua_asserterror },
	{ nullptr, nullptr }
};

static ExecOutput runConformance(LuauRuntime &runtime, const char *name) {
	// Borrowed from Luau (license information in COPYRIGHT.txt).
	std::string path = __FILE__;
	path.erase(path.find_last_of("\\/") + 1);
	path += name;

	std::ifstream stream(path, std::ios::in | std::ios::binary);
	REQUIRE(stream);

	std::stringstream buffer;
	buffer << stream.rdbuf();

	lua_State *L = runtime.GetVM(CoreVM);
	lua_State *T = lua_newthread(L);
	luaL_sandboxthread(T);

	const luaL_Reg *lib = TEST_LIB;
	while (lib->func) {
		lua_pushcfunction(T, lib->func, lib->name);
		lua_setglobal(T, lib->name);

		lib++;
	}

	return luaSBX_exec(T, buffer.str().c_str());
}

#define CONFORMANCE_TEST(name, file, init)              \
	TEST_CASE("conformance: " name) {                   \
		LuauRuntime runtime(init);                      \
                                                        \
		ExecOutput out = runConformance(runtime, file); \
		if (out.status != LUA_OK)                       \
			FAIL_CHECK(out.error);                      \
	}

CONFORMANCE_TEST("enums", "DataTypes/Enums.luau", DataTypes::luaSBX_opendatatypes)
CONFORMANCE_TEST("vector3", "DataTypes/Vector3.luau", DataTypes::luaSBX_opendatatypes)
