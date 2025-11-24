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

#include "Sbx/Runtime/Logger.hpp"

#include <cstdio>
#include <ctime>
#include <string>

#include "Sbx/Runtime/Base.hpp"
#include "lua.h"
#include "lualib.h"

namespace SBX {

#define PRINT_MSG(msg, color)                                \
	{                                                        \
		time_t rawtime;                                      \
		time(&rawtime);                                      \
		tm *buf = localtime(&rawtime);                       \
		char timestr[128];                                   \
		strftime(timestr, sizeof(timestr), "%H:%M:%S", buf); \
		printf("%s -- " color "%s\x1b[0m\n", timestr, msg);  \
	}

void Logger::Print(const char *msg) const {
	PRINT_MSG(msg, "");

	for (LogCallback cb : hooks) {
		cb(LogKind::Info, msg);
	}
}

void Logger::Warn(const char *msg) const {
	PRINT_MSG(msg, "\x1b[33m");

	for (LogCallback cb : hooks) {
		cb(LogKind::Warn, msg);
	}
}

void Logger::Error(const char *msg) const {
	PRINT_MSG(msg, "\x1b[31m");

	for (LogCallback cb : hooks) {
		cb(LogKind::Error, msg);
	}
}

void Logger::AddHook(LogCallback hook) {
	hooks.insert(hook);
}

void Logger::RemoveHook(LogCallback hook) {
	hooks.erase(hook);
}

static int luaSBX_print(lua_State *L) {
	SbxThreadData *udata = luaSBX_getthreaddata(L);
	if (!udata->global->logger) {
		luaSBX_nologerror(L);
	}

	std::string output;

	int n = lua_gettop(L);
	for (int i = 1; i <= n; i++) {
		const char *str = lua_tostring(L, i);
		if (i > 1) {
			output += ' ';
		}

		output += str;
	}

	udata->global->logger->Print(output.c_str());
	return 0;
}

static int luaSBX_warn(lua_State *L) {
	SbxThreadData *udata = luaSBX_getthreaddata(L);
	if (!udata->global->logger) {
		luaSBX_nologerror(L);
	}

	std::string output;

	int n = lua_gettop(L);
	for (int i = 1; i <= n; i++) {
		const char *str = lua_tostring(L, i);
		if (i > 1) {
			output += ' ';
		}

		output += str;
	}

	udata->global->logger->Warn(output.c_str());
	return 0;
}

static const luaL_Reg LOGGER_LIB[] = {
	{ "print", luaSBX_print },
	{ "warn", luaSBX_warn },

	{ nullptr, nullptr }
};

void luaSBX_openlogger(lua_State *L) {
	luaL_register(L, "_G", LOGGER_LIB);
	lua_pop(L, 1); // table
}

} //namespace SBX
