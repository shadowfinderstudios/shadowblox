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
#include <unordered_map>

#include "lua.h"

#include "Sbx/Runtime/Base.hpp"

namespace SBX {

class LuauRuntime;
class SignalEmitter;

// Luau uses 1 "step unit" ~= 1KiB
// amount (bytes) = step << 10
#define GC_RATE_MIN 50
#define GC_RATE_INC 25
#define GC_RATE_MAX 10000

// https://create.roblox.com/docs/scripting/events/deferred#resumption-points
enum class ResumptionPoint : uint8_t {
	Input,
	PreRender,
	LegacyWait,
	PreAnimation,
	PreSimulation,
	PostSimulation,
	Wait,
	Heartbeat,
	BindToClose
};

class ScheduledTask {
public:
	ScheduledTask(lua_State *T);
	virtual ~ScheduledTask();

	ScheduledTask(const ScheduledTask &other) = delete;
	ScheduledTask(ScheduledTask &&other) = delete;
	ScheduledTask &operator=(const ScheduledTask &other) = delete;
	ScheduledTask &operator=(ScheduledTask &&other) = delete;

	lua_State *GetThread() { return T; }

	virtual bool CanThrottle() { return false; }
	virtual bool IsComplete(ResumptionPoint point) = 0;
	virtual bool ShouldResume() { return true; }
	virtual int PushResults() = 0;
	virtual void Update(uint64_t frame, double delta) {} // NOLINT

private:
	lua_State *T;
	int threadRef;
};

struct DeferredEvent {
	// Not to be dereferenced; may be collected prior to firing event
	void *emitter;
	uint64_t id;
	lua_State *L;
	std::function<void()> resume;

	std::unordered_map<SignalEmitter *, std::unordered_map<uint64_t, int>> pathReentrancy;
};

class TaskScheduler {
public:
	TaskScheduler(LuauRuntime *runtime);

	void AddTask(ScheduledTask *task);
	bool AddDeferredEvent(SignalEmitter *emitter, uint64_t id, lua_State *L, std::function<void()> resume);
	void CancelTask(ScheduledTask *task);
	void CancelThread(lua_State *L);
	void CancelEvents(SignalEmitter *emitter, uint64_t id);

	int NumPendingTasks() const;
	int NumPendingEvents() const;

	void Resume(ResumptionPoint point, uint64_t frame, double delta, double throttleThreshold);
	void GCStep(double delta);

	const uint32_t *GetGCStepSize() const { return gcCollectRate; }

private:
	LuauRuntime *runtime;

	std::list<ScheduledTask *> tasks;
	std::list<DeferredEvent> deferredEvents;

	std::unordered_map<SignalEmitter *, std::unordered_map<uint64_t, int>> currentReentrancy;

	uint32_t gcCollectRate[VMMax] = { GC_RATE_MIN };
	int32_t gcSizeRate[VMMax] = { 0 };
	int32_t gcLastSize[VMMax] = { 0 };
};

void luaSBX_opensched(lua_State *L);

} //namespace SBX
