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

#include "lua.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Logger.hpp"
#include "Utils.hpp"

using namespace SBX;

TEST_SUITE_BEGIN("Runtime/Logger");

TEST_CASE("C++ functionality") {
	Logger logger;
	static int hit = 0;
	logger.AddHook([](LogKind, const char *) {
		hit++;
	});

	logger.Print("This is a test print: %d", 1234);
	logger.Warn("This is a test warn: %d", 1234);
	logger.Error("This is a test error: %d", 1234);

	CHECK_EQ(hit, 3);
}

TEST_CASE("Luau functionality") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	Logger logger;
	static int hit = 0;
	logger.AddHook([](LogKind, const char *) {
		hit++;
	});

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	udata->global->logger = &logger;

	CHECK_EVAL_FAIL(L, R"ASDF(
	    print("a", "b", "c", 123)
		warn("a", "b", "c", 123)
		error("Oops!")
	)ASDF",
			"exec:4: Oops!")

	CHECK_EQ(hit, 3);
	luaSBX_close(L);
}

TEST_SUITE_END();
