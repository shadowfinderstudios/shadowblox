#include "doctest.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"
#include "lua.h"

TEST_SUITE_BEGIN("Runtime/LuauRuntime");

TEST_CASE("initialize") {
	static int hit = 0;
	LuauRuntime rt([](lua_State *L) {
		hit++;
	});

	CHECK_EQ(hit, LuauRuntime::VM_MAX);

	auto LC = rt.getVM(LuauRuntime::CoreVM);
	SbxThreadData *udataCore = luaSBX_getthreaddata(LC);
	REQUIRE_NE(udataCore, nullptr);
	CHECK_EQ(udataCore->vmType, LuauRuntime::CoreVM);
	CHECK_EQ(udataCore->identity, ElevatedGameScriptIdentity);

	auto LU = rt.getVM(LuauRuntime::UserVM);
	SbxThreadData *udataUser = luaSBX_getthreaddata(LU);
	REQUIRE_NE(udataUser, nullptr);
	CHECK_EQ(udataUser->vmType, LuauRuntime::UserVM);
	CHECK_EQ(udataUser->identity, GameScriptIdentity);

	// Ensure acquisition of mutex
	CHECK_FALSE(udataCore->mutex->try_lock());
	CHECK_FALSE(udataUser->mutex->try_lock());
}

TEST_SUITE_END();
