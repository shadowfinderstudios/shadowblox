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

#include "doctest.h"

#include <optional>
#include <string>

#include "ltm.h"
#include "lua.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/ClassBinder.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"
#include "Sbx/Runtime/Stack.hpp"
#include "Utils.hpp"

using namespace SBX;

TEST_SUITE_BEGIN("Runtime/ClassBinder");

struct TestStruct2 {
	int num;

	static TestStruct2 New(std::optional<int> num) {
		return TestStruct2(num.value_or(42));
	}

	TestStruct2() :
			num(0) {}

	TestStruct2(int num) :
			num(num) {}

	int GetNum() const {
		return num;
	}

	void SetNum(int newNum) {
		num = newNum;
	}

	int Square() const {
		return num * num;
	}

	int Len() const {
		return num;
	}

	void operator()(int inc) {
		num += inc;
	}
};

TestStruct2 operator+(const TestStruct2 &x, const TestStruct2 &y) {
	return TestStruct2(x.num + y.num);
}

TestStruct2 operator+(const TestStruct2 &x, int y) {
	return TestStruct2(x.num + y);
}

TestStruct2 operator+(int y, const TestStruct2 &x) {
	return TestStruct2(x.num + y);
}

TestStruct2 operator*(const TestStruct2 &x, int y) {
	return TestStruct2(x.num * y);
}

STACK_OP_UDATA_DEF(TestStruct2);
UDATA_STACK_OP_IMPL(TestStruct2, "SbxTests.TestStruct2", Test1Udata, NO_DTOR);

TEST_CASE("example") {
	using Binder = LuauClassBinder<TestStruct2>;

	lua_State *L = luaSBX_newstate(LuauRuntime::CoreVM, ElevatedGameScriptIdentity);
	SbxThreadData *udata = luaSBX_getthreaddata(L);

	Binder::Init("TestStruct", "SbxTests.TestStruct", Test1Udata);

	Binder::BindStaticMethod<"new", TestStruct2::New, NoneSecurity>();
	Binder::BindMethod<"Square", &TestStruct2::Square, NoneSecurity>();
	Binder::BindMethod<"SetNum", &TestStruct2::SetNum, InternalTestSecurity>();
	Binder::BindProperty<"Num", &TestStruct2::GetNum, NoneSecurity, &TestStruct2::SetNum, InternalTestSecurity>();
	Binder::BindPropertyReadOnly<"SqNum", &TestStruct2::Square, NoneSecurity>();
	Binder::BindPropertyWriteOnly<"NumProxy", &TestStruct2::SetNum, InternalTestSecurity>();
	Binder::BindCallOp<&TestStruct2::operator(), InternalTestSecurity>();
	Binder::BindBinaryOp<TM_ADD, static_cast<TestStruct2 (*)(const TestStruct2 &, const TestStruct2 &)>(operator+)>(LuauStackOp<TestStruct2>::Is, LuauStackOp<TestStruct2>::Is);
	Binder::BindBinaryOp<TM_ADD, static_cast<TestStruct2 (*)(const TestStruct2 &, int)>(operator+)>(LuauStackOp<TestStruct2>::Is, LuauStackOp<int>::Is);
	Binder::BindBinaryOp<TM_ADD, static_cast<TestStruct2 (*)(int, const TestStruct2 &)>(operator+)>(LuauStackOp<int>::Is, LuauStackOp<TestStruct2>::Is);
	Binder::BindBinaryOp<TM_MUL, static_cast<TestStruct2 (*)(const TestStruct2 &, int)>(operator*)>(LuauStackOp<TestStruct2>::Is, LuauStackOp<int>::Is);
	Binder::BindUnaryOp<TM_LEN, &TestStruct2::Len>();

	Binder::InitMetatable(L);
	Binder::InitGlobalTable(L);

	SUBCASE("basic functionality") {
		EVAL_THEN(L, "return TestStruct.new()", {
			TestStruct2 *ts = LuauStackOp<TestStruct2 *>::Get(L, -1);
			CHECK_EQ(ts->num, 42);
		});

		CHECK_EVAL_EQ(L, "return getmetatable(TestStruct.new())", std::string, "The metatable is locked");

		EVAL_THEN(L, "return TestStruct.new(1234)", {
			TestStruct2 *ts = LuauStackOp<TestStruct2 *>::Get(L, -1);
			CHECK_EQ(ts->num, 1234);
		});

		EVAL_THEN(L, "return TestStruct.new(5):Square()", {
			CHECK_EQ(lua_tonumber(L, -1), 25);
		});

		EVAL_THEN(L, "local x = TestStruct.new(5); x:SetNum(2); return x", {
			TestStruct2 *ts = LuauStackOp<TestStruct2 *>::Get(L, -1);
			CHECK_EQ(ts->num, 2);
		});

		EVAL_THEN(L, "return TestStruct.new(5).SqNum", {
			CHECK_EQ(lua_tonumber(L, -1), 25);
		});

		EVAL_THEN(L, R"ASDF(local x = TestStruct.new(5)
x.NumProxy = 1
return x
)ASDF",
				{
					TestStruct2 *ts = LuauStackOp<TestStruct2 *>::Get(L, -1);
					CHECK_EQ(ts->num, 1);
				});

		EVAL_THEN(L, "return TestStruct.new(21) + 21", {
			TestStruct2 *ts = LuauStackOp<TestStruct2 *>::Get(L, -1);
			CHECK_EQ(ts->num, 42);
		});

		EVAL_THEN(L, "return 21 + TestStruct.new(21)", {
			TestStruct2 *ts = LuauStackOp<TestStruct2 *>::Get(L, -1);
			CHECK_EQ(ts->num, 42);
		});

		EVAL_THEN(L, "local x = TestStruct.new(21); x(5); return x", {
			TestStruct2 *ts = LuauStackOp<TestStruct2 *>::Get(L, -1);
			CHECK_EQ(ts->num, 26);
		});

		EVAL_THEN(L, "return TestStruct.new(21) * 2", {
			TestStruct2 *ts = LuauStackOp<TestStruct2 *>::Get(L, -1);
			CHECK_EQ(ts->num, 42);
		});

		EVAL_THEN(L, "return #(TestStruct.new(5))", {
			CHECK_EQ(lua_tonumber(L, -1), 5);
		});
	}

	SUBCASE("invalid argument") {
		CHECK_EVAL_FAIL(L, "TestStruct.new(5):SetNum(nil)", "exec:1: invalid argument #2 (number expected, got nil)");
	}

	SUBCASE("invalid method") {
		CHECK_EVAL_FAIL(L, "TestStruct.new(5):AHOIGHOISDHOGO()", "exec:1: AHOIGHOISDHOGO is not a valid member of TestStruct");
	}

	SUBCASE("read-only") {
		CHECK_EVAL_FAIL(L, "TestStruct.new(5).SqNum = 9", "exec:1: SqNum cannot be assigned to");
	}

	SUBCASE("write-only") {
		CHECK_EVAL_FAIL(L, "return TestStruct.new(5).NumProxy", "exec:1: NumProxy cannot be read");
	}

	SUBCASE("nonexistent read") {
		CHECK_EVAL_FAIL(L, "return TestStruct.new(5).AHIHGIH", "exec:1: AHIHGIH is not a valid member of TestStruct");
	}

	SUBCASE("nonexistent write") {
		CHECK_EVAL_FAIL(L, "TestStruct.new(5).AHIHGIH = 9", "exec:1: AHIHGIH cannot be assigned to");
	}

	SUBCASE("invalid operator overload") {
		CHECK_EVAL_FAIL(L, "return 8 * TestStruct.new(5)", "exec:1: attempt to perform arithmetic (mul) on number and TestStruct");
	}

	SUBCASE("no permission to call") {
		udata->identity = GameScriptIdentity;
		CHECK_EVAL_FAIL(L, "TestStruct.new(5):SetNum(2)", "exec:1: The current identity (2) cannot call 'SetNum' (lacking permission 17)");
	}

	SUBCASE("no permission to write") {
		udata->identity = GameScriptIdentity;
		CHECK_EVAL_FAIL(L, "TestStruct.new(5).Num = 9", "exec:1: The current identity (2) cannot read 'Num' (lacking permission 17)");
	}

	SUBCASE("no permission to use operator") {
		udata->identity = GameScriptIdentity;
		CHECK_EVAL_FAIL(L, "(TestStruct.new())()", "exec:1: The current identity (2) cannot use operator (lacking permission 17)");
	}

	luaSBX_close(L);
}

TEST_SUITE_END();
