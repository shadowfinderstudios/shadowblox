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

#include "Sbx/Classes/Model.hpp"
#include "Sbx/Classes/Part.hpp"
#include "Sbx/DataTypes/Vector3.hpp"

namespace SBX::Classes {

/**
 * @brief This class implements Roblox's [`Workspace`](https://create.roblox.com/docs/reference/engine/classes/Workspace)
 * class.
 *
 * Workspace is a service that contains all objects in the 3D world. It holds
 * Parts, Models, and other physical objects. The 'workspace' global variable
 * refers to the Workspace service.
 */
class Workspace : public Model {
	SBXCLASS(Workspace, Model, MemoryCategory::Instances, ClassTag::NotCreatable, ClassTag::Service);

public:
	Workspace();
	~Workspace() override;

	// Physics properties
	DataTypes::Vector3 GetGravity() const { return gravity; }
	void SetGravity(DataTypes::Vector3 value);

	double GetFallenPartsDestroyHeight() const { return fallenPartsDestroyHeight; }
	void SetFallenPartsDestroyHeight(double value);

	// Streaming properties
	bool GetStreamingEnabled() const { return streamingEnabled; }
	void SetStreamingEnabled(bool value);

	// Client visibility radius
	double GetStreamingMinRadius() const { return streamingMinRadius; }
	void SetStreamingMinRadius(double value);

	double GetStreamingTargetRadius() const { return streamingTargetRadius; }
	void SetStreamingTargetRadius(double value);

	// Terrain (placeholder - to be implemented later)
	// std::shared_ptr<Terrain> GetTerrain() const;

	// CurrentCamera (placeholder - to be implemented later)
	// std::shared_ptr<Camera> GetCurrentCamera() const;
	// void SetCurrentCamera(std::shared_ptr<Camera> camera);

	// DistributedGameTime (read-only)
	double GetDistributedGameTime() const { return distributedGameTime; }
	void UpdateDistributedGameTime(double time) { distributedGameTime = time; }

protected:
	template <typename T>
	static void BindMembers() {
		Model::BindMembers<T>();

		// Physics properties
		ClassDB::BindProperty<T, "Gravity", "Physics",
				&Workspace::GetGravity, NoneSecurity,
				&Workspace::SetGravity, NoneSecurity,
				ThreadSafety::Unsafe, true, true>({});

		ClassDB::BindProperty<T, "FallenPartsDestroyHeight", "Physics",
				&Workspace::GetFallenPartsDestroyHeight, NoneSecurity,
				&Workspace::SetFallenPartsDestroyHeight, NoneSecurity,
				ThreadSafety::Unsafe, true, true>({});

		// Streaming properties
		ClassDB::BindProperty<T, "StreamingEnabled", "Streaming",
				&Workspace::GetStreamingEnabled, NoneSecurity,
				&Workspace::SetStreamingEnabled, NoneSecurity,
				ThreadSafety::Unsafe, true, true>({});

		ClassDB::BindProperty<T, "StreamingMinRadius", "Streaming",
				&Workspace::GetStreamingMinRadius, NoneSecurity,
				&Workspace::SetStreamingMinRadius, NoneSecurity,
				ThreadSafety::Unsafe, true, true>({});

		ClassDB::BindProperty<T, "StreamingTargetRadius", "Streaming",
				&Workspace::GetStreamingTargetRadius, NoneSecurity,
				&Workspace::SetStreamingTargetRadius, NoneSecurity,
				ThreadSafety::Unsafe, true, true>({});

		// Read-only properties
		ClassDB::BindPropertyReadOnly<T, "DistributedGameTime", "Data",
				&Workspace::GetDistributedGameTime, NoneSecurity,
				ThreadSafety::Safe, false>({});
	}

private:
	// Physics
	DataTypes::Vector3 gravity = DataTypes::Vector3(0, -196.2, 0);  // Default Roblox gravity
	double fallenPartsDestroyHeight = -500.0;

	// Streaming
	bool streamingEnabled = false;
	double streamingMinRadius = 64.0;
	double streamingTargetRadius = 1024.0;

	// Time
	double distributedGameTime = 0.0;
};

} // namespace SBX::Classes
