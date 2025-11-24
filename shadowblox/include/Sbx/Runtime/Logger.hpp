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
