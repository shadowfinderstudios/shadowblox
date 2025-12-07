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

#include <memory>

#include "lua.h"

#include "Sbx/Classes/Instance.hpp"

namespace SBX::Classes {

/**
 * @brief This class implements Roblox's [`RunService`](https://create.roblox.com/docs/reference/engine/classes/RunService)
 * class.
 *
 * RunService contains methods and events for time-management as well as for managing
 * the context in which a game or script is running.
 */
class RunService : public Instance {
	SBXCLASS(RunService, Instance, MemoryCategory::Instances, ClassTag::NotCreatable, ClassTag::Service);

public:
	RunService();
	~RunService() override;

	// Methods
	bool IsClient() const;
	bool IsServer() const;
	bool IsStudio() const;
	bool IsRunning() const;
	bool IsRunMode() const;
	bool IsEdit() const;

	// Get the time elapsed since the last frame
	double GetDeltaTime() const { return deltaTime; }

	// Fire signals (called by engine each frame)
	void FireStepped(double time, double dt);
	void FireHeartbeat(double dt);
	void FireRenderStepped(double dt);

	// Pause/Resume (for editor)
	void Pause();
	void Run();
	void Stop();

	// Context setters (called during initialization)
	void SetIsClient(bool value) { isClient = value; }
	void SetIsServer(bool value) { isServer = value; }
	void SetIsStudio(bool value) { isStudio = value; }
	void SetIsRunMode(bool value) { isRunMode = value; }
	void SetIsEdit(bool value) { isEdit = value; }

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();

		// Methods
		ClassDB::BindMethod<T, "IsClient", &RunService::IsClient, NoneSecurity, ThreadSafety::Safe>({});
		ClassDB::BindMethod<T, "IsServer", &RunService::IsServer, NoneSecurity, ThreadSafety::Safe>({});
		ClassDB::BindMethod<T, "IsStudio", &RunService::IsStudio, NoneSecurity, ThreadSafety::Safe>({});
		ClassDB::BindMethod<T, "IsRunning", &RunService::IsRunning, NoneSecurity, ThreadSafety::Safe>({});
		ClassDB::BindMethod<T, "IsRunMode", &RunService::IsRunMode, NoneSecurity, ThreadSafety::Safe>({});
		ClassDB::BindMethod<T, "IsEdit", &RunService::IsEdit, NoneSecurity, ThreadSafety::Safe>({});

		// Signals
		// Stepped(time: number, deltaTime: number) - Fires before physics
		ClassDB::BindSignal<T, "Stepped", void(double, double), NoneSecurity>({}, "time", "deltaTime");

		// Heartbeat(deltaTime: number) - Fires after physics
		ClassDB::BindSignal<T, "Heartbeat", void(double), NoneSecurity>({}, "deltaTime");

		// RenderStepped(deltaTime: number) - Fires before rendering (client only)
		ClassDB::BindSignal<T, "RenderStepped", void(double), NoneSecurity>({}, "deltaTime");

		// PreAnimation(deltaTime: number) - Fires before animation
		ClassDB::BindSignal<T, "PreAnimation", void(double), NoneSecurity>({}, "deltaTime");

		// PreRender(deltaTime: number) - Fires before rendering
		ClassDB::BindSignal<T, "PreRender", void(double), NoneSecurity>({}, "deltaTime");

		// PreSimulation(deltaTime: number) - Fires before physics simulation
		ClassDB::BindSignal<T, "PreSimulation", void(double), NoneSecurity>({}, "deltaTime");

		// PostSimulation(deltaTime: number) - Fires after physics simulation
		ClassDB::BindSignal<T, "PostSimulation", void(double), NoneSecurity>({}, "deltaTime");
	}

private:
	bool isClient = false;
	bool isServer = true;  // Default to server context
	bool isStudio = false;
	bool isRunning = false;
	bool isRunMode = false;
	bool isEdit = true;
	double deltaTime = 0.0;
	double elapsedTime = 0.0;
};

} // namespace SBX::Classes
