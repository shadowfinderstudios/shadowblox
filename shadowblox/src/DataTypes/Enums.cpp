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

#include "Sbx/DataTypes/Enums.hpp"

#include <vector>

#include "lua.h"
#include "lualib.h" // NOLINT

#include "Sbx/DataTypes/Enum.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/ClassBinder.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

void Enums::Register(lua_State *L) {
	using B = LuauClassBinder<Enums>;

	if (!B::IsInitialized()) {
		B::Init("Enums", "Enums", EnumsUdata);
		B::SetIndexOverride(IndexOverride);
		B::BindToString<&Enums::ToString>();
		B::BindMethod<"GetEnums", &Enums::GetEnums, NoneSecurity>();
	}

	B::InitMetatable(L);
}

const char *Enums::ToString() const {
	return "Enums";
}

const std::vector<Enum *> &Enums::GetEnums() const {
	return enums;
}

void Enums::AddEnum(Enum *enumType) {
	enums.push_back(enumType);
	nameToEnum[enumType->GetName()] = enumType;
}

int Enums::IndexOverride(lua_State *L, const char *name) {
	const Enums *self = LuauStackOp<Enums *>::Check(L, 1);
	auto it = self->nameToEnum.find(name);

	if (it != self->nameToEnum.end()) {
		LuauStackOp<Enum *>::Push(L, it->second);
		return 1;
	}

	return 0;
}

} //namespace SBX::DataTypes

namespace SBX {

REGISTRY_PTR_STACK_OP_IMPL(DataTypes::Enums, "Enums", "Enums", EnumsUdata, false);

}
