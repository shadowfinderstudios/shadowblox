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

#include "Sbx/Runtime/LuauRuntime.hpp"

#include <cstdint>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Base.hpp"

namespace SBX {

LuauRuntime::LuauRuntime(void (*initCallback)(lua_State *), bool debug) :
		initCallback(initCallback) {
	vms[CoreVM] = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	vms[UserVM] = luaSBX_newstate(UserVM, GameScriptIdentity);

	double init = lua_clock();
	for (int i = 0; i < VMMax; i++) {
		SbxThreadData *udata = luaSBX_getthreaddata(vms[i]);
		udata->global->initTimestamp = init;

		InitVM(vms[i], debug);
	}
}

LuauRuntime::~LuauRuntime() {
	for (lua_State *&L : vms) {
		luaSBX_close(L);
		L = nullptr;
	}
}

void LuauRuntime::InitVM(lua_State *L, bool debug) {
	if (debug) {
		luaSBX_debugcallbacks(L);
	}

	if (initCallback) {
		initCallback(L);
	}

	// Seal main global state
	luaL_sandbox(L);
}

lua_State *LuauRuntime::GetVM(VMType type) {
	return vms[type];
}

void LuauRuntime::GCStep(const uint32_t *step, double delta) {
	for (int i = 0; i < VMMax; i++) {
		lua_State *L = GetVM(VMType(i));
		lua_gc(L, LUA_GCSTEP, step[i] * delta);
	}
}

void LuauRuntime::GCSize(int32_t *outBuffer) {
	for (int i = 0; i < VMMax; i++) {
		lua_State *L = GetVM(VMType(i));
		outBuffer[i] = lua_gc(L, LUA_GCCOUNT, 0);
	}
}

} //namespace SBX
