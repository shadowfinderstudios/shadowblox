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

#include "lua.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"

using namespace SBX;

TEST_SUITE_BEGIN("Runtime/LuauRuntime");

TEST_CASE("initialize") {
	static int hit = 0;
	LuauRuntime rt([](lua_State *L) {
		hit++;
	});

	CHECK_EQ(hit, VMMax);

	lua_State *LC = rt.GetVM(CoreVM);
	SbxThreadData *udataCore = luaSBX_getthreaddata(LC);
	REQUIRE_NE(udataCore, nullptr);
	CHECK_EQ(lua_gettop(LC), 0);
	CHECK_EQ(udataCore->vmType, CoreVM);
	CHECK_EQ(udataCore->identity, ElevatedGameScriptIdentity);

	lua_State *LU = rt.GetVM(UserVM);
	SbxThreadData *udataUser = luaSBX_getthreaddata(LU);
	REQUIRE_NE(udataUser, nullptr);
	CHECK_EQ(lua_gettop(LU), 0);
	CHECK_EQ(udataUser->vmType, UserVM);
	CHECK_EQ(udataUser->identity, GameScriptIdentity);
}

TEST_SUITE_END();
