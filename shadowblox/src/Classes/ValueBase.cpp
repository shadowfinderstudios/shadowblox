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

#include "Sbx/Classes/ValueBase.hpp"

#include <cstring>

#include "lua.h"

#include "Sbx/Runtime/Stack.hpp"

namespace SBX::Classes {

// ValueBase
ValueBase::ValueBase() :
		Instance() {
	SetName("Value");
}

// StringValue
StringValue::StringValue() :
		ValueBase() {
	SetName("StringValue");
}

void StringValue::SetValue(const char *newValue) {
	value = newValue ? newValue : "";
	Changed<StringValue>("Value");
}

// IntValue
IntValue::IntValue() :
		ValueBase() {
	SetName("IntValue");
}

void IntValue::SetValue(int64_t newValue) {
	value = newValue;
	Changed<IntValue>("Value");
}

// NumberValue
NumberValue::NumberValue() :
		ValueBase() {
	SetName("NumberValue");
}

void NumberValue::SetValue(double newValue) {
	value = newValue;
	Changed<NumberValue>("Value");
}

// BoolValue
BoolValue::BoolValue() :
		ValueBase() {
	SetName("BoolValue");
}

void BoolValue::SetValue(bool newValue) {
	value = newValue;
	Changed<BoolValue>("Value");
}

// ObjectValue
ObjectValue::ObjectValue() :
		ValueBase() {
	SetName("ObjectValue");
}

void ObjectValue::SetValue(std::shared_ptr<Instance> newValue) {
	value = newValue;
	Changed<ObjectValue>("Value");
}

int ObjectValue::GetValueLuau(lua_State *L) {
	ObjectValue *self = LuauStackOp<ObjectValue *>::Check(L, 1);
	std::shared_ptr<Instance> val = self->GetValue();
	if (val) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, val);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ObjectValue::SetValueLuau(lua_State *L) {
	ObjectValue *self = LuauStackOp<ObjectValue *>::Check(L, 1);
	if (lua_isnil(L, 3)) {
		self->SetValue(nullptr);
	} else {
		std::shared_ptr<Instance> val = LuauStackOp<std::shared_ptr<Instance>>::Check(L, 3);
		self->SetValue(val);
	}
	return 0;
}

int ObjectValue::ObjectValueIndexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "Value") == 0) {
		return GetValueLuau(L);
	}
	return 0;
}

bool ObjectValue::ObjectValueNewindexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "Value") == 0) {
		SetValueLuau(L);
		return true;
	}
	return false;
}

} // namespace SBX::Classes
