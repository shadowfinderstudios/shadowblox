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

#pragma once

#include <string>

#include "doctest.h"
#include "lua.h"

struct ExecOutput {
	int status;
	std::string error;
};

ExecOutput luaGD_exec(lua_State *L, const char *src);

#define EVAL_THEN(L, src, expr)                              \
	{                                                        \
		int top = lua_gettop(L);                             \
		ExecOutput out = luaGD_exec(L, src);                 \
                                                             \
		if (out.status != LUA_OK && out.status != LUA_YIELD) \
			FAIL_CHECK(out.error);                           \
                                                             \
		expr;                                                \
                                                             \
		lua_settop(L, top);                                  \
	}

#define CHECK_EVAL_EQ(L, src, type, value)                      \
	EVAL_THEN(L, src, {                                         \
		CHECK(::SBX::LuauStackOp<type>::Check(L, -1) == value); \
	})

#define CHECK_EVAL_OK(L, src) EVAL_THEN(L, src, {})

#define CHECK_EVAL_FAIL(L, src, err)         \
	{                                        \
		ExecOutput out = luaGD_exec(L, src); \
                                             \
		REQUIRE_NE(out.status, LUA_OK);      \
		INFO(out.error);                     \
		CHECK_EQ(out.error, err);            \
	}
