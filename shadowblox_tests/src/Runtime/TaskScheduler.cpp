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
#include "Sbx/Runtime/TaskScheduler.hpp"
#include "Utils.hpp"

using namespace SBX;

TEST_SUITE_BEGIN("Runtime/TaskScheduler");

TEST_CASE("resume") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	TaskScheduler scheduler(nullptr);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	udata->global->scheduler = &scheduler;

	SUBCASE("legacy wait") {
		CHECK_EVAL_OK(L, R"ASDF(
			local t, u = wait(1)
			assert(t > 1)
			assert(u > 0)
		)ASDF")

		REQUIRE_EQ(lua_status(L), LUA_YIELD);

		// Throttle
		scheduler.Resume(ResumptionPoint::Heartbeat, 1, 0.5, -1.0);
		REQUIRE_EQ(lua_status(L), LUA_YIELD);

		// Pass
		scheduler.Resume(ResumptionPoint::Heartbeat, 2, 0.6, 1.0);
		REQUIRE_EQ(lua_status(L), LUA_OK);
	}

	luaSBX_close(L);
}

TEST_CASE("VM closed") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	TaskScheduler scheduler(nullptr);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	udata->global->scheduler = &scheduler;

	CHECK_EVAL_OK(L, "wait(1)");
	CHECK_EQ(lua_status(L), LUA_YIELD);
	CHECK_EQ(scheduler.NumPendingTasks(), 1);

	luaSBX_close(L);

	CHECK_EQ(scheduler.NumPendingTasks(), 0);
}

TEST_CASE("wait") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	TaskScheduler scheduler(nullptr);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	udata->global->scheduler = &scheduler;

	SUBCASE("legacy") {
		CHECK_EVAL_OK(L, R"ASDF(
    		local t, u = wait(1)
    		assert(t > 1)
    		assert(u > 0)
		)ASDF")

		REQUIRE_EQ(lua_status(L), LUA_YIELD);

		// Odd frame
		scheduler.Resume(ResumptionPoint::Heartbeat, 1, 2.0, 1.0);
		REQUIRE_EQ(lua_status(L), LUA_YIELD);

		// Even frame
		scheduler.Resume(ResumptionPoint::Heartbeat, 2, 2.0, 1.0);
		REQUIRE_EQ(lua_status(L), LUA_OK);
	}

	SUBCASE("task") {
		CHECK_EVAL_OK(L, R"ASDF(
    		local t, u = task.wait(1)
    		assert(t > 1)
    		assert(u == nil)
    	)ASDF")

		REQUIRE_EQ(lua_status(L), LUA_YIELD);

		// Immediate resumption
		scheduler.Resume(ResumptionPoint::Heartbeat, 1, 2.0, 1.0);
		REQUIRE_EQ(lua_status(L), LUA_OK);
	}

	SUBCASE("pcall") {
		CHECK_EVAL_OK(L, R"ASDF(
		    pcall(function()
				local t, u = task.wait(1)
				error("Oh no!")
			end)
		)ASDF")

		REQUIRE_EQ(lua_status(L), LUA_YIELD);

		scheduler.Resume(ResumptionPoint::Heartbeat, 1, 2.0, 1.0);
		// OK despite error
		REQUIRE_EQ(lua_status(L), LUA_OK);
	}

	luaSBX_close(L);
}

TEST_SUITE_END();
