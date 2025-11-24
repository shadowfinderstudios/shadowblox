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

#include <string>

#include "lua.h"

#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

class Enum;

/**
 * @brief This class implements Roblox's [`EnumItem`](https://create.roblox.com/docs/en-us/reference/engine/datatypes/EnumItem)
 * data type.
 */
class EnumItem {
public:
	EnumItem(const char *name, int value, Enum *enumType);

	static void Register(lua_State *L);

	const char *GetName() const { return name; }
	int GetValue() const { return value; }
	Enum *GetEnumType() const { return enumType; }

	std::string ToString() const;

private:
	const char *name;
	int value;
	Enum *enumType;
};

} //namespace SBX::DataTypes

namespace SBX {

STACK_OP_REGISTRY_PTR_DEF(DataTypes::EnumItem);

}
