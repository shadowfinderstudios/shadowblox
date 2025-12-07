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

#include <cmath>
#include <string>

#include "lua.h"

#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

/**
 * @brief This class implements Roblox's [`Color3`](https://create.roblox.com/docs/en-us/reference/engine/datatypes/Color3)
 * data type.
 *
 * A Color3 represents a color in RGB color space, with each component ranging from 0 to 1.
 */
class Color3 {
public:
	double R = 0.0;
	double G = 0.0;
	double B = 0.0;

	// Constructors
	Color3() = default;
	Color3(double r, double g, double b);

	static void Register(lua_State *L);

	// Static constructors
	static Color3 FromRGB(int r, int g, int b);
	static Color3 FromHSV(double h, double s, double v);
	static Color3 FromHex(const std::string &hex);

	// Properties (getters for Luau binding)
	double GetR() const { return R; }
	double GetG() const { return G; }
	double GetB() const { return B; }

	// Methods
	Color3 Lerp(const Color3 &goal, double alpha) const;
	std::tuple<double, double, double> ToHSV() const;
	std::string ToHex() const;

	std::string ToString() const;

	// Operators
	Color3 operator+(const Color3 &other) const;
	Color3 operator-(const Color3 &other) const;
	Color3 operator*(const Color3 &other) const;
	Color3 operator*(double scalar) const;
	Color3 operator/(double scalar) const;

	bool operator==(const Color3 &other) const;
};

// Scalar * Color3
inline Color3 operator*(double scalar, const Color3 &col) {
	return col * scalar;
}

} //namespace SBX::DataTypes

namespace SBX {

STACK_OP_UDATA_DEF(DataTypes::Color3);

} //namespace SBX
