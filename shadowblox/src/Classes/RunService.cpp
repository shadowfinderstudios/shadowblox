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

#include "Sbx/Classes/RunService.hpp"

#include "Sbx/Classes/ClassDB.hpp"

namespace SBX::Classes {

RunService::RunService() :
		Instance() {
	SetName("RunService");
}

RunService::~RunService() {
}

bool RunService::IsClient() const {
	return isClient;
}

bool RunService::IsServer() const {
	return isServer;
}

bool RunService::IsStudio() const {
	return isStudio;
}

bool RunService::IsRunning() const {
	return isRunning;
}

bool RunService::IsRunMode() const {
	return isRunMode;
}

bool RunService::IsEdit() const {
	return isEdit;
}

void RunService::FireStepped(double time, double dt) {
	if (!isRunning) {
		return;
	}

	deltaTime = dt;
	elapsedTime = time;
	Emit<RunService>("Stepped", time, dt);
	Emit<RunService>("PreSimulation", dt);
}

void RunService::FireHeartbeat(double dt) {
	if (!isRunning) {
		return;
	}

	deltaTime = dt;
	Emit<RunService>("Heartbeat", dt);
	Emit<RunService>("PostSimulation", dt);
}

void RunService::FireRenderStepped(double dt) {
	if (!isRunning || !isClient) {
		return;
	}

	deltaTime = dt;
	Emit<RunService>("RenderStepped", dt);
	Emit<RunService>("PreRender", dt);
	Emit<RunService>("PreAnimation", dt);
}

void RunService::Pause() {
	isRunning = false;
	isRunMode = false;
}

void RunService::Run() {
	isRunning = true;
	isRunMode = true;
	isEdit = false;
}

void RunService::Stop() {
	isRunning = false;
	isRunMode = false;
	isEdit = true;
}

} // namespace SBX::Classes
