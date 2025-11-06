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

#pragma once

#include <cstdint>
#include <cstdio>
#include <unordered_set>

#include "lua.h"

namespace SBX {

#define MAX_FMT_LOG_SIZE 256

enum class LogKind : uint8_t {
	Info,
	Warn,
	Error,
};

/**
 * @brief Class to manage log messages and callbacks.
 */
class Logger {
public:
	using LogCallback = void (*)(LogKind kind, const char *msg);

	void Print(const char *msg) const;
	void Warn(const char *msg) const;
	void Error(const char *msg) const;

	template <typename... Args>
	void Print(const char *str, Args... args) const {
		char output[MAX_FMT_LOG_SIZE];
		snprintf(output, MAX_FMT_LOG_SIZE, str, args...);
		Print(output);
	}

	template <typename... Args>
	void Warn(const char *str, Args... args) const {
		char output[MAX_FMT_LOG_SIZE];
		snprintf(output, MAX_FMT_LOG_SIZE, str, args...);
		Warn(output);
	}

	template <typename... Args>
	void Error(const char *str, Args... args) const {
		char output[MAX_FMT_LOG_SIZE];
		snprintf(output, MAX_FMT_LOG_SIZE, str, args...);
		Error(output);
	}

	/**
	 * @brief Add a hook to run on every log message.
	 *
	 * The hook must not assume that the message is a stable pointer. If needed,
	 * make a copy of the given string.
	 *
	 * @param hook The hook function.
	 */
	void AddHook(LogCallback hook);
	void RemoveHook(LogCallback hook);

private:
	std::unordered_set<LogCallback> hooks;
};

void luaSBX_openlogger(lua_State *L);

} //namespace SBX
