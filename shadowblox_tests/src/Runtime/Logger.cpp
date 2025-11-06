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
