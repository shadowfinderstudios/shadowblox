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
#include "Utils.hpp"

using namespace SBX;

TEST_SUITE_BEGIN("Runtime/Base");

TEST_CASE("newstate") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	REQUIRE_NE(L, nullptr);
	CHECK_EQ(lua_gettop(L), 0);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	CHECK_EQ(udata->vmType, CoreVM);
	CHECK_EQ(udata->identity, ElevatedGameScriptIdentity);
	CHECK_NE(udata->global, nullptr);

	luaSBX_close(L);
}

TEST_CASE("newthread") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	REQUIRE_NE(L, nullptr);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	REQUIRE_NE(udata, nullptr);

	SUBCASE("lua_newthread") {
		lua_State *T = lua_newthread(L);
		REQUIRE_NE(T, nullptr);

		SbxThreadData *threadUdata = luaSBX_getthreaddata(T);
		REQUIRE_NE(threadUdata, nullptr);

		CHECK_EQ(threadUdata->vmType, udata->vmType);
		CHECK_EQ(threadUdata->identity, udata->identity);
		CHECK_EQ(threadUdata->global, udata->global);

		lua_pop(L, 1);
	}

	SUBCASE("luaSBX_newthread") {
		lua_State *T = luaSBX_newthread(L, AnonymousIdentity);
		REQUIRE_NE(T, nullptr);

		SbxThreadData *threadUdata = luaSBX_getthreaddata(T);
		REQUIRE_NE(threadUdata, nullptr);

		CHECK_EQ(threadUdata->vmType, udata->vmType);
		CHECK_EQ(threadUdata->identity, AnonymousIdentity);
		CHECK_EQ(threadUdata->global, udata->global);

		lua_pop(L, 1);
	}

	luaSBX_close(L);
}

TEST_CASE("permissions") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	REQUIRE_NE(L, nullptr);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	REQUIRE_NE(udata, nullptr);
	CHECK(luaSBX_iscapability(L, RobloxScriptSecurity));
	CHECK_FALSE(luaSBX_iscapability(L, NotAccessibleSecurity));

	udata->identity = GameScriptIdentity;
	CHECK_FALSE(luaSBX_iscapability(L, RobloxScriptSecurity));

	udata->additionalCapability = RobloxScriptSecurity;
	CHECK(luaSBX_iscapability(L, RobloxScriptSecurity));
	udata->additionalCapability = NoneSecurity;

	udata->identity = COMIdentity;
	CHECK(luaSBX_iscapability(L, NotAccessibleSecurity));

	luaSBX_close(L);
}

TEST_CASE("registry") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	REQUIRE_NE(L, nullptr);

	auto push = [](lua_State *L, void *ptr, void *userdata) {
		lua_newuserdata(L, 1);
	};

	SUBCASE("strong") {
		CHECK(luaSBX_pushregistry(L, nullptr, nullptr, push, false));
		CHECK_FALSE(luaSBX_pushregistry(L, nullptr, nullptr, push, false));
		CHECK_EQ(lua_gettop(L), 2);

		CHECK(lua_equal(L, -2, -1));
		lua_pop(L, 2);
		lua_gc(L, LUA_GCCOLLECT, 0);

		CHECK_FALSE(luaSBX_pushregistry(L, nullptr, nullptr, push, false));
		lua_pop(L, 1);
	}

	SUBCASE("weak") {
		CHECK(luaSBX_pushregistry(L, nullptr, nullptr, push, true));
		CHECK_FALSE(luaSBX_pushregistry(L, nullptr, nullptr, push, true));
		CHECK_EQ(lua_gettop(L), 2);

		CHECK(lua_equal(L, -2, -1));
		lua_pop(L, 2);
		lua_gc(L, LUA_GCCOLLECT, 0);

		CHECK(luaSBX_pushregistry(L, nullptr, nullptr, push, false));
		lua_pop(L, 1);
	}

	luaSBX_close(L);
}

TEST_CASE("timeout") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	luaSBX_debugcallbacks(L);

	CHECK_EVAL_FAIL(L, "while true do end", "Script timed out: exhausted allowed execution time");

	luaSBX_close(L);
}

TEST_SUITE_END();
