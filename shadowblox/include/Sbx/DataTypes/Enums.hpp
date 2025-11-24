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

#include <vector>

#include "lua.h"

#include "Sbx/Runtime/Stack.hpp"
#include "Sbx/Runtime/StringMap.hpp"

namespace SBX::DataTypes {

class Enum;

/**
 * @brief This class implements Roblox's [`Enums`](https://create.roblox.com/docs/en-us/reference/engine/datatypes/Enums)
 * data type.
 */
class Enums {
public:
	static void Register(lua_State *L);

	const char *ToString() const;

	const std::vector<Enum *> &GetEnums() const;

protected:
	friend class Enum;
	void AddEnum(Enum *enumType);

private:
	static int IndexOverride(lua_State *L, const char *name);

	std::vector<Enum *> enums;
	StringMap<Enum *> nameToEnum;
};

} //namespace SBX::DataTypes

namespace SBX {

STACK_OP_REGISTRY_PTR_DEF(DataTypes::Enums);

}
