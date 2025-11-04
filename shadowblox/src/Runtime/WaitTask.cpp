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

#include "Sbx/Runtime/WaitTask.hpp"

#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/TaskScheduler.hpp"

struct lua_State;

namespace SBX {

WaitTask::WaitTask(lua_State *T, double duration, bool canThrottle) :
		ScheduledTask(T), remaining(duration), canThrottle(canThrottle) {
	start = lua_clock();
}

int WaitTask::IsComplete(ResumptionPoint) {
	return remaining == 0.0;
}

int WaitTask::PushResults() {
	lua_pushnumber(T, lua_clock() - start);

	if (canThrottle) {
		// TODO: Correctness
		lua_pushnumber(T, lua_clock());
		return 2;
	}

	return 1;
}

void WaitTask::Update(double delta) {
	if (delta > remaining) {
		remaining = 0.0;
	} else {
		remaining -= delta;
	}
}

int luaSBX_wait(lua_State *L) {
	SbxThreadData *udata = luaSBX_getthreaddata(L);
	double duration = luaL_checknumber(L, 1);

	if (!udata->global->scheduler)
		luaSBX_noschederror(L);

	WaitTask *task = new WaitTask(L, duration, true);
	udata->global->scheduler->AddTask(task);

	return lua_yield(L, 0);
}

} //namespace SBX
