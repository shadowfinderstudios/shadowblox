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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX {

/**
 * @brief Throw this error inside functions bound to Luau to trigger a
 * luaL_error.
 * @see luaSBX_bindcxx
 */
class LuauBinderError : public std::exception {};

template <typename T>
inline T luaSBX_checkarg(lua_State *L, int &index) {
	return LuauStackOp<T>::Check(L, index++);
}

template <>
inline lua_State *luaSBX_checkarg<lua_State *>(lua_State *L, int &index) {
	return L;
}

// https://stackoverflow.com/a/70954691
template <typename Sig>
struct FuncType;

template <typename R, typename... Args>
struct FuncType<R (*)(Args...)> {
	using FuncPtrType = R (*)(Args...);
	using ClassType = void;
	using RetType = R;
	using ArgTypes = std::tuple<Args...>;

	static inline R Invoke(lua_State *L, FuncPtrType func) {
		// C++ FUN FACT:
		// (): Parameter pack expansion may not be evaluated in order
		// {}: Evaluated in order
		// https://stackoverflow.com/a/42047998
		int i = 1;
		return std::apply(func, std::tuple{ luaSBX_checkarg<Args>(L, i)... });
	}
};

template <typename T, typename R, typename... Args>
struct FuncType<R (T::*)(Args...)> {
	using FuncPtrType = R (T::*)(Args...);
	using ClassType = T;
	using RetType = R;
	using ArgTypes = std::tuple<Args...>;

	template <size_t... N>
	static inline R Invoke(lua_State *L, FuncPtrType func, std::index_sequence<N...>) {
		int i = 2;
		auto self = LuauStackOp<T *>::Check(L, 1);
		std::tuple args{ luaSBX_checkarg<Args>(L, i)... };
		return std::invoke(func, self, std::get<N>(args)...);
	}

	static inline R Invoke(lua_State *L, FuncPtrType func) {
		return Invoke(L, func, std::make_index_sequence<sizeof...(Args)>());
	}
};

template <typename T, typename R, typename... Args>
struct FuncType<R (T::*)(Args...) const> {
	using FuncPtrType = R (T::*)(Args...) const;
	using ClassType = T;
	using RetType = R;
	using ArgTypes = std::tuple<Args...>;

	template <size_t... N>
	static inline R Invoke(lua_State *L, FuncPtrType func, std::index_sequence<N...>) {
		int i = 2;
		auto self = LuauStackOp<T *>::Check(L, 1);
		std::tuple args{ luaSBX_checkarg<Args>(L, i)... };
		return std::invoke(func, self, std::get<N>(args)...);
	}

	static inline R Invoke(lua_State *L, FuncPtrType func) {
		return Invoke(L, func, std::make_index_sequence<sizeof...(Args)>());
	}
};

// https://www.reddit.com/r/cpp_questions/comments/pumi9r/comment/he3swe8
template <auto N>
struct StringLiteral {
	char value[N];

	constexpr StringLiteral(const char (&str)[N]) {
		std::copy(str, str + N, value);
	}
};

// https://stackoverflow.com/a/48458312
template <typename>
struct IsTuple : std::false_type {};

template <typename... T>
struct IsTuple<std::tuple<T...>> : std::true_type {
	template <size_t... N>
	static inline int Push(lua_State *L, const std::tuple<T...> &val, std::index_sequence<N...>) {
		(LuauStackOp<T>::Push(L, std::get<N>(val)), ...);
		return sizeof...(T);
	}

	static inline int Push(lua_State *L, const std::tuple<T...> &val) {
		return Push(L, val, std::make_index_sequence<sizeof...(T)>());
	}
};

enum BindPurpose : uint8_t {
	BindFunction,
	BindGetter,
	BindSetter,
	BindOperator
};

/**
 * @brief A template function that converts a C++ function to a function
 * directly bindable to Luau (`lua_CFunction`).
 *
 * The function may be a non-member, static member, const member, or non-const
 * member function. If the function returns a `std::tuple`, it will be pushed
 * to Luau as separate return values.
 *
 * @tparam name       The name of the function/property to use for capability
 *                    checks.
 * @tparam F          The function.
 * @tparam capability Capabilities required to run this function.
 * @tparam TF         The type of the function. Automatically populated.
 * @tparam purpose    The purpose of this bound function (e.g., function or
 *                    property getter/setter). The capability error message is
 *                    adjusted accordingly.
 */
template <StringLiteral name, auto F, SbxCapability capability, BindPurpose purpose = BindFunction, typename TF = decltype(F)>
int luaSBX_bindcxx(lua_State *L) {
	if constexpr (capability != NoneSecurity) {
		static const char *purposes[] = {
			"call",
			"write",
			"read",
			"use operator"
		};

		if constexpr (name.value[0]) {
			static std::string action = std::string(purposes[purpose]) + " '" + name.value + '\'';
			luaSBX_checkcapability(L, capability, action.c_str());
		} else {
			luaSBX_checkcapability(L, capability, purposes[purpose]);
		}
	}

	try {
		if constexpr (std::is_same<typename FuncType<TF>::RetType, void>()) {
			FuncType<TF>::Invoke(L, F);
			return 0;
		} else if constexpr (IsTuple<typename FuncType<TF>::RetType>::value) {
			return IsTuple<typename FuncType<TF>::RetType>::Push(L, FuncType<TF>::Invoke(L, F));
		} else {
			LuauStackOp<typename FuncType<TF>::RetType>::Push(L, FuncType<TF>::Invoke(L, F));
			return 1;
		}
	} catch (LuauBinderError &e) {
		luaL_error(L, "%s", e.what());
	}
}

} //namespace SBX
