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

#include "lua.h"

#include "Sbx/Runtime/TaskScheduler.hpp"

namespace SBX {

class WaitTask : public ScheduledTask {
public:
	WaitTask(lua_State *T, double duration, bool legacyThrottling);

	bool CanThrottle() override { return legacyThrottling; }
	bool IsComplete(ResumptionPoint /*point*/) override;
	int PushResults() override;
	void Update(uint64_t frame, double delta) override;

private:
	double elapsed;
	double duration;
	uint64_t lastFrame;
	bool legacyThrottling;
};

int luaSBX_wait(lua_State *L);
int luaSBX_taskwait(lua_State *L);

} //namespace SBX
