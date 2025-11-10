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
