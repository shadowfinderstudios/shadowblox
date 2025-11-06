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
	if (!udata->global->logger)
		luaSBX_nologerror(L);

	std::string output;

	int n = lua_gettop(L);
	for (int i = 1; i <= n; i++) {
		const char *str = lua_tostring(L, i);
		if (i > 1)
			output += ' ';
		output += str;
	}

	udata->global->logger->Print(output.c_str());
	return 0;
}

static int luaSBX_warn(lua_State *L) {
	SbxThreadData *udata = luaSBX_getthreaddata(L);
	if (!udata->global->logger)
		luaSBX_nologerror(L);

	std::string output;

	int n = lua_gettop(L);
	for (int i = 1; i <= n; i++) {
		const char *str = lua_tostring(L, i);
		if (i > 1)
			output += ' ';
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
