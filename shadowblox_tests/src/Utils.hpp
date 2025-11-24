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

#pragma once

#include <string>

#include "doctest.h"
#include "lua.h"

struct ExecOutput {
	int status;
	std::string error;
};

ExecOutput luaSBX_exec(lua_State *L, const char *src);

#define EVAL_THEN(L, src, expr)                              \
	{                                                        \
		int top = lua_gettop(L);                             \
		ExecOutput out = luaSBX_exec(L, src);                \
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

#define CHECK_EVAL_FAIL(L, src, err)          \
	{                                         \
		ExecOutput out = luaSBX_exec(L, src); \
                                              \
		REQUIRE_NE(out.status, LUA_OK);       \
		INFO(out.error);                      \
		CHECK_EQ(out.error, err);             \
	}
