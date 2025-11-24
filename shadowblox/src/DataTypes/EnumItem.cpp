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

#include "Sbx/DataTypes/EnumItem.hpp"

#include <string>

#include "lua.h"

#include "Sbx/Classes/Variant.hpp"
#include "Sbx/DataTypes/Enum.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/ClassBinder.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

EnumItem::EnumItem(const char *name, int value, Enum *enumType) :
		name(name), value(value), enumType(enumType) {
	enumType->AddItem(this);
}

void EnumItem::Register(lua_State *L) {
	using B = LuauClassBinder<EnumItem>;

	if (!B::IsInitialized()) {
		B::Init("EnumItem", "EnumItem", EnumItemUdata, Classes::Variant::EnumItem);
		B::BindToString<&EnumItem::ToString>();
		B::BindPropertyReadOnly<"Name", &EnumItem::GetName, NoneSecurity>();
		B::BindPropertyReadOnly<"Value", &EnumItem::GetValue, NoneSecurity>();
		B::BindPropertyReadOnly<"EnumType", &EnumItem::GetEnumType, NoneSecurity>();
	}

	B::InitMetatable(L);
}

std::string EnumItem::ToString() const {
	return std::string("Enum.") + enumType->name + "." + name;
}

} //namespace SBX::DataTypes

namespace SBX {

REGISTRY_PTR_STACK_OP_IMPL(DataTypes::EnumItem, "EnumItem", "EnumItem", EnumItemUdata, false);

}
