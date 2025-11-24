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

#include <optional>
#include <unordered_map>
#include <vector>

#include "Sbx/Runtime/StringMap.hpp"
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
	StringMap<EnumItem *> nameToItem;
	std::unordered_map<int, EnumItem *> valueToItem;
};

} //namespace SBX::DataTypes

namespace SBX {

STACK_OP_REGISTRY_PTR_DEF(DataTypes::Enum);

}
