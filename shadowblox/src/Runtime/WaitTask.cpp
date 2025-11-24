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

#include "Sbx/Runtime/WaitTask.hpp"

#include <cstdint>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/TaskScheduler.hpp"

struct lua_State;

namespace SBX {

WaitTask::WaitTask(lua_State *T, double duration, bool legacyThrottling) :
		ScheduledTask(T), elapsed(0.0), duration(duration), lastFrame(0), legacyThrottling(legacyThrottling) {
}

bool WaitTask::IsComplete(ResumptionPoint /*point*/) {
	// Legacy wait only attempts resume every other frame
	return (!legacyThrottling || lastFrame % 2 == 0) && elapsed >= duration;
}

int WaitTask::PushResults() {
	lua_pushnumber(GetThread(), elapsed);

	if (legacyThrottling) {
		SbxThreadData *udata = luaSBX_getthreaddata(GetThread());
		lua_pushnumber(GetThread(), lua_clock() - udata->global->initTimestamp);
		return 2;
	}

	return 1;
}

void WaitTask::Update(uint64_t frame, double delta) {
	elapsed += delta;
	lastFrame = frame;
}

int luaSBX_wait(lua_State *L) {
	SbxThreadData *udata = luaSBX_getthreaddata(L);
	if (!udata->global->scheduler) {
		luaSBX_noschederror(L);
	}

	double duration = luaL_checknumber(L, 1);
	WaitTask *task = new WaitTask(L, duration, true);
	udata->global->scheduler->AddTask(task);

	return lua_yield(L, 0);
}

int luaSBX_taskwait(lua_State *L) {
	SbxThreadData *udata = luaSBX_getthreaddata(L);
	if (!udata->global->scheduler) {
		luaSBX_noschederror(L);
	}

	double duration = luaL_checknumber(L, 1);
	WaitTask *task = new WaitTask(L, duration, false);
	udata->global->scheduler->AddTask(task);

	return lua_yield(L, 0);
}

} //namespace SBX
