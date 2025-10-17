#include "doctest.h"

#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"
#include "Sbx/Runtime/Stack.hpp"

TEST_SUITE_BEGIN("Runtime/Stack");

struct TestStruct {
	int value = 0;

	bool operator==(const TestStruct &other) const {
		return value == other.value;
	}
};

STACK_OP_PTR_DEF(TestStruct);
UDATA_STACK_OP_IMPL(TestStruct, "SbxTests.TestStruct", Test1Udata, NO_DTOR);

template <typename T, typename U>
static inline void testStackOp(lua_State *L, U value) {
	int top = lua_gettop(L);

	LuauStackOp<T>::push(L, value);
	CHECK_EQ(lua_gettop(L), top + 1);

	CHECK(LuauStackOp<T>::is(L, -1));
	CHECK_EQ(lua_gettop(L), top + 1);

	CHECK_EQ(LuauStackOp<T>::get(L, -1), value);
	CHECK_EQ(lua_gettop(L), top + 1);

	CHECK_EQ(LuauStackOp<T>::check(L, -1), value);
	CHECK_EQ(lua_gettop(L), top + 1);

	lua_pop(L, 1);
}

TEST_CASE("basic") {
	lua_State *L = luaSBX_newstate(LuauRuntime::CoreVM, AnonymousIdentity);

	testStackOp<bool>(L, true);
	testStackOp<int>(L, 12);
	testStackOp<std::string>(L, "hello there! おはようございます");

	luaSBX_close(L);
}

TEST_CASE("int64") {
	lua_State *L = luaSBX_newstate(LuauRuntime::CoreVM, AnonymousIdentity);

	SUBCASE("|x| <= 2^53") {
		int64_t i = 9007199254740992;
		testStackOp<int64_t>(L, i);

		LuauStackOp<int64_t>::push(L, i);
		REQUIRE(lua_isnumber(L, -1));
		lua_pop(L, 1);
	}

	SUBCASE("|x| > 2^53") {
		int64_t j = 9007199254740993;
		testStackOp<int64_t>(L, j);

		LuauStackOp<int64_t>::push(L, j);
		REQUIRE(lua_isuserdata(L, -1));
		lua_pop(L, 1);
	}

	luaSBX_close(L);
}

TEST_CASE("udata") {
	lua_State *L = luaSBX_newstate(LuauRuntime::CoreVM, AnonymousIdentity);

	lua_newtable(L);
	lua_setreadonly(L, -1, true);
	lua_setuserdatametatable(L, Test1Udata);

	TestStruct x{ 42 };
	testStackOp<TestStruct>(L, x);

	luaSBX_close(L);
}

TEST_SUITE_END();
