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
		B::AddIndexOverride(IndexOverride);
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
