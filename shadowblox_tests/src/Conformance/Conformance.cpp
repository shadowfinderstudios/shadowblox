// SPDX-License-Identifier: LGPL-3.0-or-later
/*******************************************************************************
 * shadowblox - https://git.seki.pw/Fumohouse/shadowblox
 *
 * Copyright 2025-present ksk.
 * Copyright 2025-present shadowblox contributors.
 *
 * Licensed under the GNU Lesser General Public License version 3.0 or later.
 * See COPYRIGHT.txt for more details.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
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

	ThreadHandle L = runtime.GetVM(CoreVM);
	lua_State *T = lua_newthread(L);
	luaL_sandboxthread(T);

	const luaL_Reg *lib = TEST_LIB;
	while (lib->func) {
		lua_pushcfunction(T, lib->func, lib->name);
		lua_setglobal(T, lib->name);

		lib++;
	}

	return luaGD_exec(T, buffer.str().c_str());
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
