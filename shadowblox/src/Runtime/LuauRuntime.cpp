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

	// NOTE: Main VM is NOT sandboxed because:
	// 1. It serves as a template that holds global environment (game, workspace, etc.)
	// 2. Globals are registered after init via RegisterGlobals()
	// 3. Script execution creates threads via lua_newthread() which inherit these globals
	// 4. Those threads ARE sandboxed via luaL_sandboxthread() in execute_script()
	// This is the proper Roblox-like architecture where the main state is a container
	// and individual script threads are sandboxed.
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
