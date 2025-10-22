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

#include "Sbx/DataTypes/Enum.hpp"

#include <optional>
#include <vector>

#include "lua.h"
#include "lualib.h" // NOLINT

#include "Sbx/DataTypes/EnumItem.hpp"
#include "Sbx/DataTypes/Enums.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/ClassBinder.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

Enum::Enum(const char *name, Enums *enums) :
		name(name) {
	enums->AddEnum(this);
}

void Enum::Register(lua_State *L) {
	using B = LuauClassBinder<Enum>;

	if (!B::IsInitialized()) {
		B::Init("Enum", "Enum", EnumUdata);
		B::SetIndexOverride(IndexOverride);
		B::BindToString<&Enum::ToString>();
		B::BindMethod<"GetEnumItems", &Enum::GetEnumItems, NoneSecurity>();
		B::BindMethod<"FromName", &Enum::FromName, NoneSecurity>();
		B::BindMethod<"FromValue", &Enum::FromValue, NoneSecurity>();
	}

	B::InitMetatable(L);
}

const char *Enum::ToString() const {
	return name;
}

const char *Enum::GetName() const {
	return name;
}

const std::vector<EnumItem *> &Enum::GetEnumItems() const {
	return items;
}

std::optional<EnumItem *> Enum::FromName(const char *name) {
	auto it = nameToItem.find(name);
	if (it != nameToItem.end())
		return it->second;
	else
		return std::nullopt;
}

std::optional<EnumItem *> Enum::FromValue(int value) {
	auto it = valueToItem.find(value);
	if (it != valueToItem.end())
		return it->second;
	else
		return std::nullopt;
}

void Enum::AddItem(EnumItem *item) {
	items.push_back(item);
	nameToItem[item->GetName()] = item;
	valueToItem[item->GetValue()] = item;
}

int Enum::IndexOverride(lua_State *L, const char *name) {
	const Enum *self = LuauStackOp<Enum *>::Check(L, 1);
	auto it = self->nameToItem.find(name);

	if (it != self->nameToItem.end()) {
		LuauStackOp<EnumItem *>::Push(L, it->second);
		return 1;
	}

	return 0;
}

} //namespace SBX::DataTypes

namespace SBX {

REGISTRY_PTR_STACK_OP_IMPL(DataTypes::Enum, "Enum", EnumUdata, false);

}
