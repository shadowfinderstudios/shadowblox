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

#include "Sbx/DataTypes/Vector3.hpp"

#include <cmath>
#include <sstream>
#include <string>

#include "ltm.h"
#include "lua.h"

#include "Sbx/Classes/Variant.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/ClassBinder.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

// Constants
const Vector3 Vector3::zero(0.0, 0.0, 0.0);
const Vector3 Vector3::one(1.0, 1.0, 1.0);
const Vector3 Vector3::xAxis(1.0, 0.0, 0.0);
const Vector3 Vector3::yAxis(0.0, 1.0, 0.0);
const Vector3 Vector3::zAxis(0.0, 0.0, 1.0);

// Constructors
Vector3::Vector3(double x, double y, double z) :
		X(x), Y(y), Z(z) {}

Vector3::Vector3(double xyz) :
		X(xyz), Y(xyz), Z(xyz) {}

// Properties
double Vector3::GetMagnitude() const {
	return std::sqrt(X * X + Y * Y + Z * Z);
}

Vector3 Vector3::GetUnit() const {
	double mag = GetMagnitude();
	if (mag == 0.0) {
		return Vector3::zero;
	}
	return Vector3(X / mag, Y / mag, Z / mag);
}

// Methods
Vector3 Vector3::Abs() const {
	return Vector3(std::abs(X), std::abs(Y), std::abs(Z));
}

Vector3 Vector3::Ceil() const {
	return Vector3(std::ceil(X), std::ceil(Y), std::ceil(Z));
}

Vector3 Vector3::Floor() const {
	return Vector3(std::floor(X), std::floor(Y), std::floor(Z));
}

Vector3 Vector3::Sign() const {
	auto signOf = [](double v) -> double {
		if (v > 0.0)
			return 1.0;
		if (v < 0.0)
			return -1.0;
		return 0.0;
	};
	return Vector3(signOf(X), signOf(Y), signOf(Z));
}

Vector3 Vector3::Cross(const Vector3 &other) const {
	return Vector3(
			Y * other.Z - Z * other.Y,
			Z * other.X - X * other.Z,
			X * other.Y - Y * other.X);
}

double Vector3::Dot(const Vector3 &other) const {
	return X * other.X + Y * other.Y + Z * other.Z;
}

double Vector3::Angle(const Vector3 &other, const Vector3 *axis) const {
	Vector3 a = GetUnit();
	Vector3 b = other.GetUnit();

	double dot = a.Dot(b);
	// Clamp to avoid NaN from acos
	dot = std::max(-1.0, std::min(1.0, dot));

	double angle = std::acos(dot);

	if (axis != nullptr) {
		Vector3 cross = a.Cross(b);
		if (cross.Dot(*axis) < 0.0) {
			angle = -angle;
		}
	}

	return angle;
}

Vector3 Vector3::Lerp(const Vector3 &goal, double alpha) const {
	return Vector3(
			X + (goal.X - X) * alpha,
			Y + (goal.Y - Y) * alpha,
			Z + (goal.Z - Z) * alpha);
}

Vector3 Vector3::Max(const Vector3 &other) const {
	return Vector3(
			std::max(X, other.X),
			std::max(Y, other.Y),
			std::max(Z, other.Z));
}

Vector3 Vector3::Min(const Vector3 &other) const {
	return Vector3(
			std::min(X, other.X),
			std::min(Y, other.Y),
			std::min(Z, other.Z));
}

bool Vector3::FuzzyEq(const Vector3 &other, double epsilon) const {
	return std::abs(X - other.X) <= epsilon &&
		   std::abs(Y - other.Y) <= epsilon &&
		   std::abs(Z - other.Z) <= epsilon;
}

std::string Vector3::ToString() const {
	std::ostringstream ss;
	ss << X << ", " << Y << ", " << Z;
	return ss.str();
}

// Operators
Vector3 Vector3::operator+(const Vector3 &other) const {
	return Vector3(X + other.X, Y + other.Y, Z + other.Z);
}

Vector3 Vector3::operator-(const Vector3 &other) const {
	return Vector3(X - other.X, Y - other.Y, Z - other.Z);
}

Vector3 Vector3::operator*(const Vector3 &other) const {
	return Vector3(X * other.X, Y * other.Y, Z * other.Z);
}

Vector3 Vector3::operator/(const Vector3 &other) const {
	return Vector3(X / other.X, Y / other.Y, Z / other.Z);
}

Vector3 Vector3::operator*(double scalar) const {
	return Vector3(X * scalar, Y * scalar, Z * scalar);
}

Vector3 Vector3::operator/(double scalar) const {
	return Vector3(X / scalar, Y / scalar, Z / scalar);
}

Vector3 Vector3::operator-() const {
	return Vector3(-X, -Y, -Z);
}

bool Vector3::operator==(const Vector3 &other) const {
	return X == other.X && Y == other.Y && Z == other.Z;
}

// Helper functions for operator bindings
static Vector3 ScalarTimesVector3(double scalar, const Vector3 &vec) {
	return vec * scalar;
}

static Vector3 NegateVector3(const Vector3 &vec) {
	return -vec;
}

static Vector3 SubtractVector3(const Vector3 &a, const Vector3 &b) {
	return a - b;
}

// Luau registration
void Vector3::Register(lua_State *L) {
	using B = LuauClassBinder<Vector3>;

	if (!B::IsInitialized()) {
		B::Init("Vector3", "Vector3", Vector3Udata, Classes::Variant::TypeMax);

		// ToString
		B::BindToString<&Vector3::ToString>();

		// Properties (read-only)
		B::BindPropertyReadOnly<"X", &Vector3::GetX, NoneSecurity>();
		B::BindPropertyReadOnly<"Y", &Vector3::GetY, NoneSecurity>();
		B::BindPropertyReadOnly<"Z", &Vector3::GetZ, NoneSecurity>();
		B::BindPropertyReadOnly<"Magnitude", &Vector3::GetMagnitude, NoneSecurity>();
		B::BindPropertyReadOnly<"Unit", &Vector3::GetUnit, NoneSecurity>();

		// Methods
		B::BindMethod<"Abs", &Vector3::Abs, NoneSecurity>();
		B::BindMethod<"Ceil", &Vector3::Ceil, NoneSecurity>();
		B::BindMethod<"Floor", &Vector3::Floor, NoneSecurity>();
		B::BindMethod<"Sign", &Vector3::Sign, NoneSecurity>();
		B::BindMethod<"Cross", &Vector3::Cross, NoneSecurity>();
		B::BindMethod<"Dot", &Vector3::Dot, NoneSecurity>();
		B::BindMethod<"Lerp", &Vector3::Lerp, NoneSecurity>();
		B::BindMethod<"Max", &Vector3::Max, NoneSecurity>();
		B::BindMethod<"Min", &Vector3::Min, NoneSecurity>();
		B::BindMethod<"FuzzyEq", &Vector3::FuzzyEq, NoneSecurity>();

		// Unary operators
		B::BindUnaryOp<TM_UNM, &NegateVector3>();

		// Binary operators - Vector3 op Vector3
		B::BindBinaryOp<TM_ADD, &Vector3::operator+>(
				LuauStackOp<Vector3>::Is, LuauStackOp<Vector3>::Is);
		B::BindBinaryOp<TM_SUB, &SubtractVector3>(
				LuauStackOp<Vector3>::Is, LuauStackOp<Vector3>::Is);
		B::BindBinaryOp<TM_MUL, static_cast<Vector3 (Vector3::*)(const Vector3 &) const>(&Vector3::operator*)>(
				LuauStackOp<Vector3>::Is, LuauStackOp<Vector3>::Is);
		B::BindBinaryOp<TM_DIV, static_cast<Vector3 (Vector3::*)(const Vector3 &) const>(&Vector3::operator/)>(
				LuauStackOp<Vector3>::Is, LuauStackOp<Vector3>::Is);
		B::BindBinaryOp<TM_EQ, &Vector3::operator==>(
				LuauStackOp<Vector3>::Is, LuauStackOp<Vector3>::Is);

		// Binary operators - Vector3 op number
		B::BindBinaryOp<TM_MUL, static_cast<Vector3 (Vector3::*)(double) const>(&Vector3::operator*)>(
				LuauStackOp<Vector3>::Is, LuauStackOp<double>::Is);
		B::BindBinaryOp<TM_DIV, static_cast<Vector3 (Vector3::*)(double) const>(&Vector3::operator/)>(
				LuauStackOp<Vector3>::Is, LuauStackOp<double>::Is);

		// Binary operators - number op Vector3 (for commutative ops)
		B::BindBinaryOp<TM_MUL, &ScalarTimesVector3>(
				LuauStackOp<double>::Is, LuauStackOp<Vector3>::Is);

		// Static constructors
		B::BindLuauStaticMethod<"new", [](lua_State *L) -> int {
			int nargs = lua_gettop(L);
			if (nargs == 0) {
				LuauStackOp<Vector3>::Push(L, Vector3());
			} else if (nargs == 1) {
				double val = luaL_checknumber(L, 1);
				LuauStackOp<Vector3>::Push(L, Vector3(val));
			} else {
				double x = luaL_checknumber(L, 1);
				double y = luaL_checknumber(L, 2);
				double z = luaL_optnumber(L, 3, 0.0);
				LuauStackOp<Vector3>::Push(L, Vector3(x, y, z));
			}
			return 1;
		}>();
	}

	B::InitMetatable(L);

	// Create global table manually with constants (before making it readonly)
	lua_newtable(L);

	// Add the 'new' constructor
	lua_pushcfunction(L, [](lua_State *L) -> int {
		int nargs = lua_gettop(L);
		if (nargs == 0) {
			LuauStackOp<Vector3>::Push(L, Vector3());
		} else if (nargs == 1) {
			double val = luaL_checknumber(L, 1);
			LuauStackOp<Vector3>::Push(L, Vector3(val));
		} else {
			double x = luaL_checknumber(L, 1);
			double y = luaL_checknumber(L, 2);
			double z = luaL_optnumber(L, 3, 0.0);
			LuauStackOp<Vector3>::Push(L, Vector3(x, y, z));
		}
		return 1;
	}, "Vector3.new");
	lua_setfield(L, -2, "new");

	// Add constants
	LuauStackOp<Vector3>::Push(L, Vector3::zero);
	lua_setfield(L, -2, "zero");

	LuauStackOp<Vector3>::Push(L, Vector3::one);
	lua_setfield(L, -2, "one");

	LuauStackOp<Vector3>::Push(L, Vector3::xAxis);
	lua_setfield(L, -2, "xAxis");

	LuauStackOp<Vector3>::Push(L, Vector3::yAxis);
	lua_setfield(L, -2, "yAxis");

	LuauStackOp<Vector3>::Push(L, Vector3::zAxis);
	lua_setfield(L, -2, "zAxis");

	// Make table readonly and set as global
	lua_setreadonly(L, -1, true);
	lua_setglobal(L, "Vector3");
}

} //namespace SBX::DataTypes

namespace SBX {

UDATA_STACK_OP_IMPL(DataTypes::Vector3, "Vector3", "Vector3", Vector3Udata, NO_DTOR);

} //namespace SBX
