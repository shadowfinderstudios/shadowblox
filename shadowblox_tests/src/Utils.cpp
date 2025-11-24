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

#include "Utils.hpp"

#include <string>

#include "Luau/Compiler.h"
#include "lua.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Stack.hpp"

using namespace SBX;

ExecOutput luaSBX_exec(lua_State *L, const char *src) {
	Luau::CompileOptions opts;
	std::string bytecode = Luau::compile(src, opts);

	ExecOutput output;

	if (luau_load(L, "=exec", bytecode.data(), bytecode.size(), 0) == 0) {
		int status = luaSBX_resume(L, nullptr, 0, 1.0);
		output.status = status;

		if (status == LUA_OK) {
			return output;
		}

		if (status == LUA_YIELD) {
			INFO("thread yielded");
			return output;
		}

		std::string error = LuauStackOp<std::string>::Get(L, -1);
		lua_pop(L, 1);

		output.error = error;
		return output;
	}

	output.error = LuauStackOp<std::string>::Get(L, -1);
	lua_pop(L, 1);

	return output;
}
