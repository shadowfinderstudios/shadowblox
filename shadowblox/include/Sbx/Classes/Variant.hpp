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

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "lua.h"

#include "Sbx/DataTypes/EnumItem.hpp"
#include "Sbx/Runtime/Stack.hpp"
#include "Sbx/Runtime/StringMap.hpp"

namespace SBX::Classes {

#define VARIANT_SIZE 16

class Object;
class LuauFunction;
class Variant;

using Dictionary = StringMap<Variant>;
using Array = std::vector<Variant>;

template <typename T>
struct VariantType;

class Variant {
public:
	// Dependents:
	// - VariantType
	// - LuauStackOp
	// - methods
	enum Type : uint8_t {
		// Primitive
		Null = 0,
		Boolean,
		Integer,
		Double,
		String,

		// Luau
		Function,
		Dictionary,
		Array,

		// Data types
		EnumItem,

		// Class
		Object,

		TypeMax
	};

	Variant();
	~Variant();

	Variant(const Variant &other);
	Variant(Variant &&other) noexcept;
	Variant &operator=(const Variant &other);
	Variant &operator=(Variant &&other) noexcept;

	template <typename T>
	Variant(const T &val) {
		if constexpr (std::is_convertible_v<T, std::shared_ptr<Classes::Object>> || std::is_same_v<T, LuauFunction>) {
			if (val) {
				type = VariantType<T>::TYPE;
				Construct<T>();
				*GetPtr<T>() = val;
			} else {
				type = Null;
			}
		} else {
			type = VariantType<T>::TYPE;
			Construct<T>();
			*GetPtr<T>() = val;
		}
	}

	Variant(int8_t val) : Variant((int64_t)val) {}
	Variant(uint8_t val) : Variant((int64_t)val) {}
	Variant(int16_t val) : Variant((int64_t)val) {}
	Variant(uint16_t val) : Variant((int64_t)val) {}
	Variant(int32_t val) : Variant((int64_t)val) {}
	Variant(uint32_t val) : Variant((int64_t)val) {}

	Variant(float val) : Variant((double)val) {}

	Variant(std::string_view val) : Variant(std::string(val)) {}
	Variant(const char *val) : Variant(std::string(val)) {}

	operator bool() const { return type != Null; }

	template <typename T>
	T *GetPtr() {
		if (type != VariantType<T>::TYPE) {
			return nullptr;
		}

		if constexpr (sizeof(T) <= VARIANT_SIZE) {
			return (T *)data.opaque;
		} else {
			return (T *)data.ptr;
		}
	}

	template <typename T>
	const T *GetPtr() const {
		if (type != VariantType<T>::TYPE) {
			return nullptr;
		}

		if constexpr (sizeof(T) <= VARIANT_SIZE) {
			return (const T *)data.opaque;
		} else {
			return (const T *)data.ptr;
		}
	}

	template <typename T>
	std::optional<T> Cast() const {
		if constexpr (std::is_arithmetic_v<T>) {
			if (type == Integer) {
				return static_cast<T>(*GetPtr<int64_t>());
			}

			if (type == Double) {
				return static_cast<T>(*GetPtr<double>());
			}

			if (type == Boolean) {
				return static_cast<T>(*GetPtr<bool>());
			}

			return std::nullopt;
		} else {
			const T *ptr = GetPtr<T>();
			if (!ptr) {
				return std::nullopt;
			}

			return *ptr;
		}
	}

	template <typename T>
	std::shared_ptr<T> CastObj() const {
		const std::shared_ptr<Classes::Object> *ptr = GetPtr<std::shared_ptr<Classes::Object>>();
		return std::dynamic_pointer_cast<T>(*ptr);
	}

	void Clear();
	Type GetType() const { return type; }

private:
	friend struct LuauStackOp<Variant>;

	template <typename T>
	void Construct() {
		if constexpr (sizeof(T) <= VARIANT_SIZE) {
			new (data.opaque) T();
		} else {
			data.ptr = new T;
		}
	}

	template <typename T>
	void Destroy() {
		if constexpr (sizeof(T) <= VARIANT_SIZE) {
			((T *)data.opaque)->~T();
		} else {
			delete (T *)data.ptr;
		}
	}

	template <typename T>
	void CopyFrom(const Variant &other) {
		Construct<T>();
		*GetPtr<T>() = *other.GetPtr<T>();
	}

	template <typename T>
	void MoveFrom(Variant &&other) { // NOLINT
		if constexpr (sizeof(T) <= VARIANT_SIZE) {
			Construct<T>();
			*((T *)data.opaque) = std::move(*((T *)other.data.opaque));
		} else {
			data.ptr = other.data.ptr;
			other.data.ptr = nullptr;
		}
	}

	void Copy(const Variant &other);
	void Move(Variant &&other);

	struct Methods {
		void (Variant::*destroy)();
		void (Variant::*copyFrom)(const Variant &other);
		void (Variant::*moveFrom)(Variant &&other);
		void (*push)(lua_State *L, const Variant &value);
		Variant (*get)(lua_State *L, int index);
	};

	static const Methods METHODS[TypeMax];

	union {
		void *ptr;
		uint8_t opaque[VARIANT_SIZE];
	} data alignas(8){};

	Type type = Null;
};

#define VARIANT_TYPE_DEF(ctorType, variantType)                                                     \
	template <>                                                                                     \
	struct VariantType<ctorType> {                                                                  \
		static constexpr ::SBX::Classes::Variant::Type TYPE = ::SBX::Classes::Variant::variantType; \
	};

VARIANT_TYPE_DEF(bool, Boolean);
VARIANT_TYPE_DEF(int64_t, Integer);
VARIANT_TYPE_DEF(double, Double);
VARIANT_TYPE_DEF(std::string, String);

VARIANT_TYPE_DEF(LuauFunction, Function);
VARIANT_TYPE_DEF(Dictionary, Dictionary);
VARIANT_TYPE_DEF(Array, Array);

VARIANT_TYPE_DEF(DataTypes::EnumItem *, EnumItem);

template <typename T>
requires std::is_base_of<Object, T>::value struct VariantType<std::shared_ptr<T>> {
	static constexpr Variant::Type TYPE = Variant::Object;
};

class LuauFunction {
public:
	LuauFunction() = default; // for stack operation
	LuauFunction(lua_State *L, int index);
	~LuauFunction();

	LuauFunction(const LuauFunction &other);
	LuauFunction(LuauFunction &&other) noexcept;
	LuauFunction &operator=(const LuauFunction &other);
	LuauFunction &operator=(LuauFunction &&other) noexcept;

	operator bool() const;

	void Clear();
	bool Get(lua_State *T) const;

private:
	void Copy(const LuauFunction &other);
	void Move(LuauFunction &&other);

	lua_State *L = nullptr;
	int ref = LUA_NOREF;
};

} //namespace SBX::Classes

namespace SBX {

using Classes::Array;
using Classes::Dictionary;
using Classes::LuauFunction;
using Classes::Variant;

template <>
struct LuauStackOp<Variant> {
	static const std::string NAME;

	static void Push(lua_State *L, const Variant &value);

	static Variant::Type GetType(lua_State *L, int index);
	static Variant Get(lua_State *L, int index);
	static bool Is(lua_State *L, int index);
	static Variant Check(lua_State *L, int index);
};

STACK_OP_DEF(LuauFunction);
STACK_OP_DEF(Dictionary);
STACK_OP_DEF(Array);

} //namespace SBX
