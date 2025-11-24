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

#include <cstdint>

#include "lua.h"

#include "Sbx/Runtime/Base.hpp"

namespace SBX {

class LuauRuntime {
public:
	LuauRuntime(void (*initCallback)(lua_State *), bool debug = false);
	~LuauRuntime();

	LuauRuntime(const LuauRuntime &other) = delete;
	LuauRuntime(LuauRuntime &&other) = delete;
	LuauRuntime &operator=(const LuauRuntime &other) = delete;
	LuauRuntime &operator=(LuauRuntime &&other) = delete;

	lua_State *GetVM(VMType type);

	void GCStep(const uint32_t *step, double delta);
	void GCSize(int32_t *outBuffer);

private:
	void (*initCallback)(lua_State *);
	lua_State *vms[VMMax]{};
	void InitVM(lua_State *L, bool debug);
};

} //namespace SBX
