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
 * @brief This class implements Roblox's [`Vector3`](https://create.roblox.com/docs/en-us/reference/engine/datatypes/Vector3)
 * data type.
 *
 * A Vector3 represents a vector in 3D space, typically used as a point in 3D
 * space or the dimensions of a rectangular prism.
 */
class Vector3 {
public:
	double X = 0.0;
	double Y = 0.0;
	double Z = 0.0;

	// Constructors
	Vector3() = default;
	Vector3(double x, double y, double z);
	Vector3(double xyz); // Single value for all components

	static void Register(lua_State *L);

	// Constants
	static const Vector3 zero;
	static const Vector3 one;
	static const Vector3 xAxis;
	static const Vector3 yAxis;
	static const Vector3 zAxis;

	// Properties (getters for Luau binding)
	double GetX() const { return X; }
	double GetY() const { return Y; }
	double GetZ() const { return Z; }
	double GetMagnitude() const;
	Vector3 GetUnit() const;

	// Methods
	Vector3 Abs() const;
	Vector3 Ceil() const;
	Vector3 Floor() const;
	Vector3 Sign() const;

	Vector3 Cross(const Vector3 &other) const;
	double Dot(const Vector3 &other) const;
	double Angle(const Vector3 &other, const Vector3 *axis = nullptr) const;

	Vector3 Lerp(const Vector3 &goal, double alpha) const;
	Vector3 Max(const Vector3 &other) const;
	Vector3 Min(const Vector3 &other) const;

	bool FuzzyEq(const Vector3 &other, double epsilon = 1e-5) const;

	std::string ToString() const;

	// Operators
	Vector3 operator+(const Vector3 &other) const;
	Vector3 operator-(const Vector3 &other) const;
	Vector3 operator*(const Vector3 &other) const;
	Vector3 operator/(const Vector3 &other) const;

	Vector3 operator*(double scalar) const;
	Vector3 operator/(double scalar) const;

	Vector3 operator-() const; // Unary minus

	bool operator==(const Vector3 &other) const;
};

// Scalar * Vector3
inline Vector3 operator*(double scalar, const Vector3 &vec) {
	return vec * scalar;
}

} //namespace SBX::DataTypes

namespace SBX {

STACK_OP_UDATA_DEF(DataTypes::Vector3);

} //namespace SBX
