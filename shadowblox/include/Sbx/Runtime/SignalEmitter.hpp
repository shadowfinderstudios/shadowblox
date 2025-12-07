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
#include <functional>
#include <list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "lua.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Stack.hpp"
#include "Sbx/Runtime/StringMap.hpp"
#include "Sbx/Runtime/TaskScheduler.hpp"

namespace SBX {

#define IMMEDIATE_EVENT_REENTRANCY_LIMIT 6
#define DEFERRED_EVENT_REENTRANCY_LIMIT 79

void luaSBX_reentrancyerror(lua_State *L, const char *signalName);

class SignalWaitTask : public ScheduledTask {
public:
	using ScheduledTask::ScheduledTask;

	bool IsComplete(ResumptionPoint point) override;
	int PushResults() override;

private:
	friend class SignalEmitter;
	std::function<int(lua_State *)> pushResults;
};

class SignalEmitter {
public:
	SignalEmitter();
	~SignalEmitter();

	SignalEmitter(const SignalEmitter &other) = delete;
	SignalEmitter(SignalEmitter &&other) = delete;
	SignalEmitter &operator=(const SignalEmitter &other) = delete;
	SignalEmitter &operator=(SignalEmitter &&other) = delete;

	// Shutdown mode - when true, all signal operations are skipped
	// This prevents crashes when Luau runtime has been destroyed
	static void SetShutdownMode(bool shutdown);
	static bool IsShutdownMode();

	void SetDeferred(bool isDeferred);

	uint64_t Connect(const std::string &signal, lua_State *L, bool once);
	bool IsConnected(const std::string &signal, uint64_t id) const;
	void Disconnect(const std::string &signal, uint64_t id, bool cancel = true, bool updateOwner = true);
	int Wait(const std::string &signal, lua_State *L);

	int NumConnections() const;

	template <typename... Args>
	void Emit(std::string_view className, std::string_view signal, Args... args) {
		// Skip all signal emission during shutdown to prevent crashes
		if (shutdownMode) {
			return;
		}

		std::vector<uint64_t> toRemove;

		auto entry = connections.find(signal);
		if (entry != connections.end() && deferred) {
			// Do not fire new connections during iteration
			auto connectionsCopy = entry->second;
			for (const auto &[id, conn] : connectionsCopy) {
				SbxThreadData *udata = luaSBX_getthreaddata(conn.L);
				if (!udata->global->scheduler) {
					continue;
				}

				// Duplicate this reference: If this object is collected during
				// the resumption period, then the original conn.ref will be
				// destroyed before the resumption of the event handler.
				lua_getref(conn.L, conn.ref);
				int newRef = lua_ref(conn.L, -1);

				bool created = udata->global->scheduler->AddDeferredEvent(this, id, conn.L, [=]() {
					lua_getref(conn.L, newRef);
					(LuauStackOp<Args>::Push(conn.L, args), ...);
					luaSBX_pcall(conn.L, sizeof...(Args), 0, 0);
					lua_unref(conn.L, newRef);
				});

				if (created) {
					lua_pop(conn.L, 1); // function
				} else {
					std::string debugName = std::string(className) + '.' + std::string(signal);
					luaSBX_reentrancyerror(conn.L, debugName.c_str());
				}

				if (conn.once) {
					toRemove.push_back(id);
				}
			}
		} else if (entry != connections.end()) {
			// Do not fire new connections during iteration
			auto connectionsCopy = entry->second;
			for (const auto &[id, conn] : connectionsCopy) {
				// Deleted during iteration
				if (!entry->second.contains(id)) {
					continue;
				}

				lua_getref(conn.L, conn.ref);

				bool firstEntrant = immediateReentrancy.empty();

				if (++immediateReentrancy[id] > IMMEDIATE_EVENT_REENTRANCY_LIMIT) {
					std::string debugName = std::string(className) + '.' + std::string(signal);
					luaSBX_reentrancyerror(conn.L, debugName.c_str());
				} else {
					(LuauStackOp<Args>::Push(conn.L, args), ...);
					luaSBX_pcall(conn.L, sizeof...(Args), 0, 0);
				}

				immediateReentrancy[id]--;
				if (firstEntrant) {
					immediateReentrancy.clear();
				}

				if (conn.once) {
					toRemove.push_back(id);
				}
			}
		}

		for (uint64_t id : toRemove) {
			Disconnect(std::string(signal), id, false);
		}

		auto tasks = pendingTasks.find(signal);
		if (tasks != pendingTasks.end()) {
			for (SignalWaitTask *task : tasks->second) {
				task->pushResults = [=](lua_State *L) -> int {
					(LuauStackOp<Args>::Push(L, args), ...);
					return sizeof...(Args);
				};
			}

			tasks->second.clear();
		}
	}

private:
	struct Connection {
		lua_State *L;
		int ref;
		bool once;
	};

	static bool shutdownMode;

	bool deferred = false;
	uint64_t nextId = 0;
	std::unordered_map<uint64_t, int> immediateReentrancy;
	StringMap<std::unordered_map<uint64_t, Connection>> connections;
	StringMap<std::list<SignalWaitTask *>> pendingTasks;
};

class SignalConnectionOwner {
public:
	void Clear();
	void ClearEmitter(SignalEmitter *emitter);

protected:
	friend class SignalEmitter;
	void AddConnection(SignalEmitter *emitter, const std::string &name, uint64_t id);
	void RemoveConnection(SignalEmitter *emitter, uint64_t id);

private:
	std::unordered_map<SignalEmitter *, std::unordered_map<uint64_t, std::string>> connections;
};

} //namespace SBX
