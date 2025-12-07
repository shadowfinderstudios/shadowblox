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

#include "Sbx/Classes/Instance.hpp"

namespace SBX::Classes {

/**
 * @brief Base class for Value objects (StringValue, IntValue, NumberValue, etc.)
 */
class ValueBase : public Instance {
	SBXCLASS(ValueBase, Instance, MemoryCategory::Instances, ClassTag::NotCreatable);

public:
	ValueBase();
	~ValueBase() override = default;

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();
	}
};

/**
 * @brief This class implements Roblox's [`StringValue`](https://create.roblox.com/docs/reference/engine/classes/StringValue)
 * class.
 *
 * StringValue holds a single string value and fires Changed when it changes.
 */
class StringValue : public ValueBase {
	SBXCLASS(StringValue, ValueBase, MemoryCategory::Instances);

public:
	StringValue();
	~StringValue() override = default;

	const char *GetValue() const { return value.c_str(); }
	void SetValue(const char *newValue);

protected:
	template <typename T>
	static void BindMembers() {
		ValueBase::BindMembers<T>();

		ClassDB::BindProperty<T, "Value", "Data", &StringValue::GetValue, NoneSecurity,
				&StringValue::SetValue, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
	}

private:
	std::string value;
};

/**
 * @brief This class implements Roblox's [`IntValue`](https://create.roblox.com/docs/reference/engine/classes/IntValue)
 * class.
 */
class IntValue : public ValueBase {
	SBXCLASS(IntValue, ValueBase, MemoryCategory::Instances);

public:
	IntValue();
	~IntValue() override = default;

	int64_t GetValue() const { return value; }
	void SetValue(int64_t newValue);

protected:
	template <typename T>
	static void BindMembers() {
		ValueBase::BindMembers<T>();

		ClassDB::BindProperty<T, "Value", "Data", &IntValue::GetValue, NoneSecurity,
				&IntValue::SetValue, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
	}

private:
	int64_t value = 0;
};

/**
 * @brief This class implements Roblox's [`NumberValue`](https://create.roblox.com/docs/reference/engine/classes/NumberValue)
 * class.
 */
class NumberValue : public ValueBase {
	SBXCLASS(NumberValue, ValueBase, MemoryCategory::Instances);

public:
	NumberValue();
	~NumberValue() override = default;

	double GetValue() const { return value; }
	void SetValue(double newValue);

protected:
	template <typename T>
	static void BindMembers() {
		ValueBase::BindMembers<T>();

		ClassDB::BindProperty<T, "Value", "Data", &NumberValue::GetValue, NoneSecurity,
				&NumberValue::SetValue, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
	}

private:
	double value = 0.0;
};

/**
 * @brief This class implements Roblox's [`BoolValue`](https://create.roblox.com/docs/reference/engine/classes/BoolValue)
 * class.
 */
class BoolValue : public ValueBase {
	SBXCLASS(BoolValue, ValueBase, MemoryCategory::Instances);

public:
	BoolValue();
	~BoolValue() override = default;

	bool GetValue() const { return value; }
	void SetValue(bool newValue);

protected:
	template <typename T>
	static void BindMembers() {
		ValueBase::BindMembers<T>();

		ClassDB::BindProperty<T, "Value", "Data", &BoolValue::GetValue, NoneSecurity,
				&BoolValue::SetValue, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
	}

private:
	bool value = false;
};

/**
 * @brief This class implements Roblox's [`ObjectValue`](https://create.roblox.com/docs/reference/engine/classes/ObjectValue)
 * class.
 */
class ObjectValue : public ValueBase {
	SBXCLASS(ObjectValue, ValueBase, MemoryCategory::Instances);

public:
	ObjectValue();
	~ObjectValue() override = default;

	std::shared_ptr<Instance> GetValue() const { return value.lock(); }
	void SetValue(std::shared_ptr<Instance> newValue);

	static int GetValueLuau(lua_State *L);
	static int SetValueLuau(lua_State *L);

protected:
	template <typename T>
	static void BindMembers() {
		ValueBase::BindMembers<T>();

		// ObjectValue needs custom handling for Instance references
		LuauClassBinder<T>::AddIndexOverride(ObjectValueIndexOverride);
		LuauClassBinder<T>::AddNewindexOverride(ObjectValueNewindexOverride);

		ClassDB::BindPropertyNotScriptable<T, "Value", "Data",
				&ObjectValue::GetValue, &ObjectValue::SetValue,
				ThreadSafety::Unsafe, true, true>({});
	}

private:
	std::weak_ptr<Instance> value;

	static int ObjectValueIndexOverride(lua_State *L, const char *propName);
	static bool ObjectValueNewindexOverride(lua_State *L, const char *propName);
};

} // namespace SBX::Classes
