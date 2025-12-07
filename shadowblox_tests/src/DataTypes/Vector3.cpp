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

#include "doctest.h"

#include <cmath>

#include "lua.h"

#include "Sbx/DataTypes/Types.hpp"
#include "Sbx/DataTypes/Vector3.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Stack.hpp"
#include "Utils.hpp"

using namespace SBX;
using namespace SBX::DataTypes;

TEST_SUITE_BEGIN("DataTypes/Vector3");

TEST_CASE("constructors") {
	SUBCASE("default") {
		Vector3 v;
		CHECK_EQ(v.X, 0.0);
		CHECK_EQ(v.Y, 0.0);
		CHECK_EQ(v.Z, 0.0);
	}

	SUBCASE("xyz") {
		Vector3 v(1.0, 2.0, 3.0);
		CHECK_EQ(v.X, 1.0);
		CHECK_EQ(v.Y, 2.0);
		CHECK_EQ(v.Z, 3.0);
	}

	SUBCASE("single value") {
		Vector3 v(5.0);
		CHECK_EQ(v.X, 5.0);
		CHECK_EQ(v.Y, 5.0);
		CHECK_EQ(v.Z, 5.0);
	}
}

TEST_CASE("constants") {
	CHECK_EQ(Vector3::zero, Vector3(0.0, 0.0, 0.0));
	CHECK_EQ(Vector3::one, Vector3(1.0, 1.0, 1.0));
	CHECK_EQ(Vector3::xAxis, Vector3(1.0, 0.0, 0.0));
	CHECK_EQ(Vector3::yAxis, Vector3(0.0, 1.0, 0.0));
	CHECK_EQ(Vector3::zAxis, Vector3(0.0, 0.0, 1.0));
}

TEST_CASE("properties") {
	SUBCASE("Magnitude") {
		CHECK_EQ(Vector3(3.0, 4.0, 0.0).GetMagnitude(), 5.0);
		CHECK_EQ(Vector3::zero.GetMagnitude(), 0.0);
		CHECK_EQ(Vector3::one.GetMagnitude(), doctest::Approx(std::sqrt(3.0)));
	}

	SUBCASE("Unit") {
		Vector3 unit = Vector3(3.0, 4.0, 0.0).GetUnit();
		CHECK_EQ(unit.X, doctest::Approx(0.6));
		CHECK_EQ(unit.Y, doctest::Approx(0.8));
		CHECK_EQ(unit.Z, 0.0);
		CHECK_EQ(unit.GetMagnitude(), doctest::Approx(1.0));

		// Zero vector unit should be zero
		CHECK_EQ(Vector3::zero.GetUnit(), Vector3::zero);
	}
}

TEST_CASE("methods") {
	SUBCASE("Abs") {
		Vector3 v(-1.0, -2.0, 3.0);
		Vector3 abs = v.Abs();
		CHECK_EQ(abs.X, 1.0);
		CHECK_EQ(abs.Y, 2.0);
		CHECK_EQ(abs.Z, 3.0);
	}

	SUBCASE("Ceil") {
		Vector3 v(1.2, 2.7, -0.5);
		Vector3 ceil = v.Ceil();
		CHECK_EQ(ceil.X, 2.0);
		CHECK_EQ(ceil.Y, 3.0);
		CHECK_EQ(ceil.Z, 0.0);
	}

	SUBCASE("Floor") {
		Vector3 v(1.7, 2.2, -0.5);
		Vector3 floor = v.Floor();
		CHECK_EQ(floor.X, 1.0);
		CHECK_EQ(floor.Y, 2.0);
		CHECK_EQ(floor.Z, -1.0);
	}

	SUBCASE("Sign") {
		Vector3 v(-5.0, 0.0, 10.0);
		Vector3 sign = v.Sign();
		CHECK_EQ(sign.X, -1.0);
		CHECK_EQ(sign.Y, 0.0);
		CHECK_EQ(sign.Z, 1.0);
	}

	SUBCASE("Cross") {
		Vector3 cross = Vector3::xAxis.Cross(Vector3::yAxis);
		CHECK_EQ(cross.X, doctest::Approx(0.0));
		CHECK_EQ(cross.Y, doctest::Approx(0.0));
		CHECK_EQ(cross.Z, doctest::Approx(1.0));
	}

	SUBCASE("Dot") {
		CHECK_EQ(Vector3::xAxis.Dot(Vector3::yAxis), 0.0);
		CHECK_EQ(Vector3::xAxis.Dot(Vector3::xAxis), 1.0);
		CHECK_EQ(Vector3(1.0, 2.0, 3.0).Dot(Vector3(4.0, 5.0, 6.0)), 32.0);
	}

	SUBCASE("Lerp") {
		Vector3 a(0.0, 0.0, 0.0);
		Vector3 b(10.0, 20.0, 30.0);

		CHECK_EQ(a.Lerp(b, 0.0), a);
		CHECK_EQ(a.Lerp(b, 1.0), b);

		Vector3 mid = a.Lerp(b, 0.5);
		CHECK_EQ(mid.X, 5.0);
		CHECK_EQ(mid.Y, 10.0);
		CHECK_EQ(mid.Z, 15.0);
	}

	SUBCASE("Max") {
		Vector3 a(1.0, 5.0, 3.0);
		Vector3 b(4.0, 2.0, 6.0);
		Vector3 max = a.Max(b);
		CHECK_EQ(max.X, 4.0);
		CHECK_EQ(max.Y, 5.0);
		CHECK_EQ(max.Z, 6.0);
	}

	SUBCASE("Min") {
		Vector3 a(1.0, 5.0, 3.0);
		Vector3 b(4.0, 2.0, 6.0);
		Vector3 min = a.Min(b);
		CHECK_EQ(min.X, 1.0);
		CHECK_EQ(min.Y, 2.0);
		CHECK_EQ(min.Z, 3.0);
	}

	SUBCASE("FuzzyEq") {
		Vector3 a(1.0, 2.0, 3.0);
		Vector3 b(1.000001, 2.000001, 3.000001);
		CHECK(a.FuzzyEq(b, 1e-5));
		CHECK_FALSE(a.FuzzyEq(b, 1e-7));
	}

	SUBCASE("Angle") {
		double angle = Vector3::xAxis.Angle(Vector3::yAxis);
		CHECK_EQ(angle, doctest::Approx(M_PI / 2.0));

		double angleZero = Vector3::xAxis.Angle(Vector3::xAxis);
		CHECK_EQ(angleZero, doctest::Approx(0.0));
	}
}

TEST_CASE("operators") {
	SUBCASE("add") {
		Vector3 a(1.0, 2.0, 3.0);
		Vector3 b(4.0, 5.0, 6.0);
		Vector3 sum = a + b;
		CHECK_EQ(sum.X, 5.0);
		CHECK_EQ(sum.Y, 7.0);
		CHECK_EQ(sum.Z, 9.0);
	}

	SUBCASE("subtract") {
		Vector3 a(5.0, 7.0, 9.0);
		Vector3 b(1.0, 2.0, 3.0);
		Vector3 diff = a - b;
		CHECK_EQ(diff.X, 4.0);
		CHECK_EQ(diff.Y, 5.0);
		CHECK_EQ(diff.Z, 6.0);
	}

	SUBCASE("multiply vector") {
		Vector3 a(2.0, 3.0, 4.0);
		Vector3 b(5.0, 6.0, 7.0);
		Vector3 prod = a * b;
		CHECK_EQ(prod.X, 10.0);
		CHECK_EQ(prod.Y, 18.0);
		CHECK_EQ(prod.Z, 28.0);
	}

	SUBCASE("multiply scalar") {
		Vector3 v(1.0, 2.0, 3.0);
		Vector3 scaled = v * 2.0;
		CHECK_EQ(scaled.X, 2.0);
		CHECK_EQ(scaled.Y, 4.0);
		CHECK_EQ(scaled.Z, 6.0);

		Vector3 scaled2 = 3.0 * v;
		CHECK_EQ(scaled2.X, 3.0);
		CHECK_EQ(scaled2.Y, 6.0);
		CHECK_EQ(scaled2.Z, 9.0);
	}

	SUBCASE("divide vector") {
		Vector3 a(10.0, 20.0, 30.0);
		Vector3 b(2.0, 4.0, 5.0);
		Vector3 quot = a / b;
		CHECK_EQ(quot.X, 5.0);
		CHECK_EQ(quot.Y, 5.0);
		CHECK_EQ(quot.Z, 6.0);
	}

	SUBCASE("divide scalar") {
		Vector3 v(6.0, 9.0, 12.0);
		Vector3 scaled = v / 3.0;
		CHECK_EQ(scaled.X, 2.0);
		CHECK_EQ(scaled.Y, 3.0);
		CHECK_EQ(scaled.Z, 4.0);
	}

	SUBCASE("unary minus") {
		Vector3 v(1.0, -2.0, 3.0);
		Vector3 neg = -v;
		CHECK_EQ(neg.X, -1.0);
		CHECK_EQ(neg.Y, 2.0);
		CHECK_EQ(neg.Z, -3.0);
	}

	SUBCASE("equality") {
		Vector3 a(1.0, 2.0, 3.0);
		Vector3 b(1.0, 2.0, 3.0);
		Vector3 c(1.0, 2.0, 4.0);
		CHECK(a == b);
		CHECK_FALSE(a == c);
	}
}

TEST_CASE("stack operations") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	DataTypes::luaSBX_opendatatypes(L);

	SUBCASE("push and get") {
		Vector3 v(1.0, 2.0, 3.0);
		LuauStackOp<Vector3>::Push(L, v);
		CHECK(LuauStackOp<Vector3>::Is(L, -1));

		Vector3 got = LuauStackOp<Vector3>::Get(L, -1);
		CHECK_EQ(got.X, 1.0);
		CHECK_EQ(got.Y, 2.0);
		CHECK_EQ(got.Z, 3.0);
	}

	SUBCASE("check") {
		Vector3 v(4.0, 5.0, 6.0);
		LuauStackOp<Vector3>::Push(L, v);
		Vector3 checked = LuauStackOp<Vector3>::Check(L, -1);
		CHECK_EQ(checked.X, 4.0);
		CHECK_EQ(checked.Y, 5.0);
		CHECK_EQ(checked.Z, 6.0);
	}

	luaSBX_close(L);
}

TEST_CASE("luau integration") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	DataTypes::luaSBX_opendatatypes(L);

	SUBCASE("constructor") {
		CHECK_EVAL_OK(L, "local v = Vector3.new(1, 2, 3)");
		CHECK_EVAL_OK(L, "local v = Vector3.new()");
		CHECK_EVAL_OK(L, "local v = Vector3.new(5)");
	}

	SUBCASE("constants") {
		EVAL_THEN(L, "return Vector3.zero", {
			Vector3 v = LuauStackOp<Vector3>::Check(L, -1);
			CHECK_EQ(v, Vector3::zero);
		});

		EVAL_THEN(L, "return Vector3.one", {
			Vector3 v = LuauStackOp<Vector3>::Check(L, -1);
			CHECK_EQ(v, Vector3::one);
		});

		EVAL_THEN(L, "return Vector3.xAxis", {
			Vector3 v = LuauStackOp<Vector3>::Check(L, -1);
			CHECK_EQ(v, Vector3::xAxis);
		});
	}

	SUBCASE("properties") {
		EVAL_THEN(L, "return Vector3.new(1, 2, 3).X", {
			CHECK_EQ(lua_tonumber(L, -1), 1.0);
		});

		EVAL_THEN(L, "return Vector3.new(3, 4, 0).Magnitude", {
			CHECK_EQ(lua_tonumber(L, -1), 5.0);
		});

		EVAL_THEN(L, "return Vector3.new(3, 4, 0).Unit.X", {
			CHECK_EQ(lua_tonumber(L, -1), doctest::Approx(0.6));
		});
	}

	SUBCASE("methods") {
		EVAL_THEN(L, "return Vector3.new(1, 2, 3):Dot(Vector3.new(4, 5, 6))", {
			CHECK_EQ(lua_tonumber(L, -1), 32.0);
		});

		EVAL_THEN(L, "return Vector3.new(0, 0, 0):Lerp(Vector3.new(10, 20, 30), 0.5).X", {
			CHECK_EQ(lua_tonumber(L, -1), 5.0);
		});
	}

	SUBCASE("operators") {
		EVAL_THEN(L, "return (Vector3.new(1, 2, 3) + Vector3.new(4, 5, 6)).X", {
			CHECK_EQ(lua_tonumber(L, -1), 5.0);
		});

		EVAL_THEN(L, "return (Vector3.new(1, 2, 3) * 2).Y", {
			CHECK_EQ(lua_tonumber(L, -1), 4.0);
		});

		EVAL_THEN(L, "return (2 * Vector3.new(1, 2, 3)).Y", {
			CHECK_EQ(lua_tonumber(L, -1), 4.0);
		});

		EVAL_THEN(L, "return (-Vector3.new(1, -2, 3)).Y", {
			CHECK_EQ(lua_tonumber(L, -1), 2.0);
		});

		EVAL_THEN(L, "return Vector3.new(1, 2, 3) == Vector3.new(1, 2, 3)", {
			CHECK(lua_toboolean(L, -1));
		});

		EVAL_THEN(L, "return Vector3.new(1, 2, 3) == Vector3.new(1, 2, 4)", {
			CHECK_FALSE(lua_toboolean(L, -1));
		});
	}

	SUBCASE("tostring") {
		EVAL_THEN(L, "return tostring(Vector3.new(1, 2, 3))", {
			CHECK_EQ(std::string(lua_tostring(L, -1)), "1, 2, 3");
		});
	}

	luaSBX_close(L);
}

TEST_SUITE_END();
