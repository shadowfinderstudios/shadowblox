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

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "lua.h"

#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

class EnumItem;
class Enums;

/**
 * @brief This class implements Roblox's [`Enum`](https://create.roblox.com/docs/en-us/reference/engine/datatypes/Enum)
 * data type.
 */
class Enum {
public:
	Enum(const char *name, Enums *enums);

	static void Register(lua_State *L);

	const char *ToString() const;

	const char *GetName() const;

	const std::vector<EnumItem *> &GetEnumItems() const;
	std::optional<EnumItem *> FromName(const char *name);
	std::optional<EnumItem *> FromValue(int value);

protected:
	friend class EnumItem;
	void AddItem(EnumItem *item);

private:
	static int IndexOverride(lua_State *L, const char *name);

	const char *name;
	std::vector<EnumItem *> items;
	std::unordered_map<std::string, EnumItem *> nameToItem;
	std::unordered_map<int, EnumItem *> valueToItem;
};

} //namespace SBX::DataTypes

namespace SBX {

STACK_OP_REGISTRY_PTR_DEF(DataTypes::Enum);

}
