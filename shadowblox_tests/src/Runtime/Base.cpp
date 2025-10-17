#include "doctest.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"
#include "lua.h"

TEST_SUITE_BEGIN("Runtime/Base");

TEST_CASE("newstate") {
	lua_State *L = luaSBX_newstate(LuauRuntime::CoreVM, ElevatedGameScriptIdentity);
	REQUIRE_NE(L, nullptr);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	CHECK_EQ(udata->vmType, LuauRuntime::CoreVM);
	CHECK_EQ(udata->identity, ElevatedGameScriptIdentity);

	luaSBX_close(L);
}

TEST_CASE("newthread") {
	lua_State *L = luaSBX_newstate(LuauRuntime::CoreVM, ElevatedGameScriptIdentity);
	REQUIRE_NE(L, nullptr);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	REQUIRE_NE(udata, nullptr);

	SUBCASE("lua_newthread") {
		lua_State *T = lua_newthread(L);
		REQUIRE_NE(T, nullptr);

		SbxThreadData *threadUdata = luaSBX_getthreaddata(T);
		REQUIRE_NE(threadUdata, nullptr);

		CHECK_EQ(threadUdata->vmType, udata->vmType);
		CHECK_EQ(threadUdata->identity, udata->identity);
		CHECK_EQ(&*threadUdata->mutex, &*udata->mutex);

		lua_pop(L, 1);
	}

	SUBCASE("luaSBX_newthread") {
		lua_State *T = luaSBX_newthread(L, AnonymousIdentity);
		REQUIRE_NE(T, nullptr);

		SbxThreadData *threadUdata = luaSBX_getthreaddata(T);
		REQUIRE_NE(threadUdata, nullptr);

		CHECK_EQ(threadUdata->vmType, udata->vmType);
		CHECK_EQ(threadUdata->identity, AnonymousIdentity);
		CHECK_EQ(&*threadUdata->mutex, &*udata->mutex);

		lua_pop(L, 1);
	}

	luaSBX_close(L);
}

TEST_CASE("permissions") {
	lua_State *L = luaSBX_newstate(LuauRuntime::CoreVM, ElevatedGameScriptIdentity);
	REQUIRE_NE(L, nullptr);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	REQUIRE_NE(udata, nullptr);
	CHECK(luaSBX_iscapability(L, RobloxScriptSecurity));
	CHECK_FALSE(luaSBX_iscapability(L, NotAccessibleSecurity));

	udata->identity = GameScriptIdentity;
	CHECK_FALSE(luaSBX_iscapability(L, RobloxScriptSecurity));

	udata->additionalCapability = RobloxScriptSecurity;
	CHECK(luaSBX_iscapability(L, RobloxScriptSecurity));
	udata->additionalCapability = NoneSecurity;

	udata->identity = COMIdentity;
	CHECK(luaSBX_iscapability(L, NotAccessibleSecurity));

	luaSBX_close(L);
}

TEST_SUITE_END();
