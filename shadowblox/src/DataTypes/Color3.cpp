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

#include "Sbx/DataTypes/Color3.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

#include "ltm.h"
#include "lua.h"

#include "Sbx/Classes/Variant.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/ClassBinder.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

// Constructor
Color3::Color3(double r, double g, double b) :
		R(r), G(g), B(b) {}

// Static constructors
Color3 Color3::FromRGB(int r, int g, int b) {
	return Color3(r / 255.0, g / 255.0, b / 255.0);
}

Color3 Color3::FromHSV(double h, double s, double v) {
	// Normalize h to [0, 1)
	h = std::fmod(h, 1.0);
	if (h < 0) h += 1.0;

	double c = v * s;
	double x = c * (1.0 - std::abs(std::fmod(h * 6.0, 2.0) - 1.0));
	double m = v - c;

	double r, g, b;
	if (h < 1.0 / 6.0) {
		r = c; g = x; b = 0;
	} else if (h < 2.0 / 6.0) {
		r = x; g = c; b = 0;
	} else if (h < 3.0 / 6.0) {
		r = 0; g = c; b = x;
	} else if (h < 4.0 / 6.0) {
		r = 0; g = x; b = c;
	} else if (h < 5.0 / 6.0) {
		r = x; g = 0; b = c;
	} else {
		r = c; g = 0; b = x;
	}

	return Color3(r + m, g + m, b + m);
}

Color3 Color3::FromHex(const std::string &hex) {
	std::string h = hex;
	// Remove leading # if present
	if (!h.empty() && h[0] == '#') {
		h = h.substr(1);
	}

	unsigned int value = 0;
	std::stringstream ss;
	ss << std::hex << h;
	ss >> value;

	int r = (value >> 16) & 0xFF;
	int g = (value >> 8) & 0xFF;
	int b = value & 0xFF;

	return FromRGB(r, g, b);
}

// Methods
Color3 Color3::Lerp(const Color3 &goal, double alpha) const {
	return Color3(
			R + (goal.R - R) * alpha,
			G + (goal.G - G) * alpha,
			B + (goal.B - B) * alpha);
}

std::tuple<double, double, double> Color3::ToHSV() const {
	double maxC = std::max({R, G, B});
	double minC = std::min({R, G, B});
	double delta = maxC - minC;

	double h = 0.0;
	double s = 0.0;
	double v = maxC;

	if (delta > 0.0) {
		s = delta / maxC;

		if (maxC == R) {
			h = std::fmod((G - B) / delta, 6.0);
		} else if (maxC == G) {
			h = (B - R) / delta + 2.0;
		} else {
			h = (R - G) / delta + 4.0;
		}

		h /= 6.0;
		if (h < 0) h += 1.0;
	}

	return {h, s, v};
}

std::string Color3::ToHex() const {
	int r = static_cast<int>(std::clamp(R, 0.0, 1.0) * 255.0 + 0.5);
	int g = static_cast<int>(std::clamp(G, 0.0, 1.0) * 255.0 + 0.5);
	int b = static_cast<int>(std::clamp(B, 0.0, 1.0) * 255.0 + 0.5);

	std::ostringstream ss;
	ss << std::hex << std::uppercase << std::setfill('0')
	   << std::setw(2) << r
	   << std::setw(2) << g
	   << std::setw(2) << b;
	return ss.str();
}

std::string Color3::ToString() const {
	std::ostringstream ss;
	ss << R << ", " << G << ", " << B;
	return ss.str();
}

// Operators
Color3 Color3::operator+(const Color3 &other) const {
	return Color3(R + other.R, G + other.G, B + other.B);
}

Color3 Color3::operator-(const Color3 &other) const {
	return Color3(R - other.R, G - other.G, B - other.B);
}

Color3 Color3::operator*(const Color3 &other) const {
	return Color3(R * other.R, G * other.G, B * other.B);
}

Color3 Color3::operator*(double scalar) const {
	return Color3(R * scalar, G * scalar, B * scalar);
}

Color3 Color3::operator/(double scalar) const {
	return Color3(R / scalar, G / scalar, B / scalar);
}

bool Color3::operator==(const Color3 &other) const {
	return R == other.R && G == other.G && B == other.B;
}

// Helper functions for operator bindings
static Color3 ScalarTimesColor3(double scalar, const Color3 &col) {
	return col * scalar;
}

static Color3 SubtractColor3(const Color3 &a, const Color3 &b) {
	return a - b;
}

// Luau registration
void Color3::Register(lua_State *L) {
	using B = LuauClassBinder<Color3>;

	if (!B::IsInitialized()) {
		B::Init("Color3", "Color3", Color3Udata, Classes::Variant::TypeMax);

		// ToString
		B::BindToString<&Color3::ToString>();

		// Properties (read-only)
		B::BindPropertyReadOnly<"R", &Color3::GetR, NoneSecurity>();
		B::BindPropertyReadOnly<"G", &Color3::GetG, NoneSecurity>();
		B::BindPropertyReadOnly<"B", &Color3::GetB, NoneSecurity>();

		// Methods
		B::BindMethod<"Lerp", &Color3::Lerp, NoneSecurity>();
		B::BindMethod<"ToHex", &Color3::ToHex, NoneSecurity>();

		// ToHSV returns multiple values, needs custom binding
		B::BindLuauMethod<"ToHSV", [](lua_State *L) -> int {
			Color3 *self = LuauStackOp<Color3 *>::Get(L, 1);
			if (!self) {
				luaSBX_missingselferror(L, "ToHSV");
			}
			auto [h, s, v] = self->ToHSV();
			lua_pushnumber(L, h);
			lua_pushnumber(L, s);
			lua_pushnumber(L, v);
			return 3;
		}>();

		// Binary operators - Color3 op Color3
		B::BindBinaryOp<TM_ADD, &Color3::operator+>(
				LuauStackOp<Color3>::Is, LuauStackOp<Color3>::Is);
		B::BindBinaryOp<TM_SUB, &SubtractColor3>(
				LuauStackOp<Color3>::Is, LuauStackOp<Color3>::Is);
		B::BindBinaryOp<TM_MUL, static_cast<Color3 (Color3::*)(const Color3 &) const>(&Color3::operator*)>(
				LuauStackOp<Color3>::Is, LuauStackOp<Color3>::Is);
		B::BindBinaryOp<TM_EQ, &Color3::operator==>(
				LuauStackOp<Color3>::Is, LuauStackOp<Color3>::Is);

		// Binary operators - Color3 op number
		B::BindBinaryOp<TM_MUL, static_cast<Color3 (Color3::*)(double) const>(&Color3::operator*)>(
				LuauStackOp<Color3>::Is, LuauStackOp<double>::Is);
		B::BindBinaryOp<TM_DIV, &Color3::operator/>(
				LuauStackOp<Color3>::Is, LuauStackOp<double>::Is);

		// Binary operators - number op Color3 (for commutative ops)
		B::BindBinaryOp<TM_MUL, &ScalarTimesColor3>(
				LuauStackOp<double>::Is, LuauStackOp<Color3>::Is);

		// Static constructors
		B::BindLuauStaticMethod<"new", [](lua_State *L) -> int {
			int nargs = lua_gettop(L);
			if (nargs == 0) {
				LuauStackOp<Color3>::Push(L, Color3());
			} else {
				double r = luaL_checknumber(L, 1);
				double g = luaL_optnumber(L, 2, 0.0);
				double b = luaL_optnumber(L, 3, 0.0);
				LuauStackOp<Color3>::Push(L, Color3(r, g, b));
			}
			return 1;
		}>();

		B::BindLuauStaticMethod<"fromRGB", [](lua_State *L) -> int {
			int r = static_cast<int>(luaL_checknumber(L, 1));
			int g = static_cast<int>(luaL_checknumber(L, 2));
			int b = static_cast<int>(luaL_checknumber(L, 3));
			LuauStackOp<Color3>::Push(L, Color3::FromRGB(r, g, b));
			return 1;
		}>();

		B::BindLuauStaticMethod<"fromHSV", [](lua_State *L) -> int {
			double h = luaL_checknumber(L, 1);
			double s = luaL_checknumber(L, 2);
			double v = luaL_checknumber(L, 3);
			LuauStackOp<Color3>::Push(L, Color3::FromHSV(h, s, v));
			return 1;
		}>();

		B::BindLuauStaticMethod<"fromHex", [](lua_State *L) -> int {
			const char *hex = luaL_checkstring(L, 1);
			LuauStackOp<Color3>::Push(L, Color3::FromHex(hex));
			return 1;
		}>();
	}

	B::InitMetatable(L);

	// Create global table manually with constructors (before making it readonly)
	lua_newtable(L);

	// Add the 'new' constructor
	lua_pushcfunction(L, [](lua_State *L) -> int {
		int nargs = lua_gettop(L);
		if (nargs == 0) {
			LuauStackOp<Color3>::Push(L, Color3());
		} else {
			double r = luaL_checknumber(L, 1);
			double g = luaL_optnumber(L, 2, 0.0);
			double b = luaL_optnumber(L, 3, 0.0);
			LuauStackOp<Color3>::Push(L, Color3(r, g, b));
		}
		return 1;
	}, "Color3.new");
	lua_setfield(L, -2, "new");

	// Add fromRGB
	lua_pushcfunction(L, [](lua_State *L) -> int {
		int r = static_cast<int>(luaL_checknumber(L, 1));
		int g = static_cast<int>(luaL_checknumber(L, 2));
		int b = static_cast<int>(luaL_checknumber(L, 3));
		LuauStackOp<Color3>::Push(L, Color3::FromRGB(r, g, b));
		return 1;
	}, "Color3.fromRGB");
	lua_setfield(L, -2, "fromRGB");

	// Add fromHSV
	lua_pushcfunction(L, [](lua_State *L) -> int {
		double h = luaL_checknumber(L, 1);
		double s = luaL_checknumber(L, 2);
		double v = luaL_checknumber(L, 3);
		LuauStackOp<Color3>::Push(L, Color3::FromHSV(h, s, v));
		return 1;
	}, "Color3.fromHSV");
	lua_setfield(L, -2, "fromHSV");

	// Add fromHex
	lua_pushcfunction(L, [](lua_State *L) -> int {
		const char *hex = luaL_checkstring(L, 1);
		LuauStackOp<Color3>::Push(L, Color3::FromHex(hex));
		return 1;
	}, "Color3.fromHex");
	lua_setfield(L, -2, "fromHex");

	// Make table readonly and set as global
	lua_setreadonly(L, -1, true);
	lua_setglobal(L, "Color3");
}

} //namespace SBX::DataTypes

namespace SBX {

UDATA_STACK_OP_IMPL(DataTypes::Color3, "Color3", "Color3", Color3Udata, NO_DTOR);

} //namespace SBX
