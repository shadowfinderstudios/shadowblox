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
#include <string>
#include <unordered_map>

#include "lua.h"

#include "Sbx/Classes/Instance.hpp"

namespace SBX::Classes {

class RunService;
class Workspace;

/**
 * @brief This class implements Roblox's [`DataModel`](https://create.roblox.com/docs/reference/engine/classes/DataModel)
 * class.
 *
 * DataModel is the root of Roblox's parent-child hierarchy. It is the container
 * for all services and objects in a game. The 'game' global variable refers to
 * the DataModel instance.
 */
class DataModel : public Instance {
	SBXCLASS(DataModel, Instance, MemoryCategory::Instances, ClassTag::NotCreatable, ClassTag::NotReplicated);

public:
	DataModel();
	~DataModel() override;

	// Get a service by class name (creates if not exists)
	std::shared_ptr<Instance> GetService(const char *className);
	static int GetServiceLuau(lua_State *L);

	// Find a service by class name (does not create)
	std::shared_ptr<Instance> FindService(const char *className) const;
	static int FindServiceLuau(lua_State *L);

	// Properties
	const char *GetGameId() const { return gameId.c_str(); }
	void SetGameId(const char *id) { gameId = id ? id : ""; }

	const char *GetPlaceId() const { return placeId.c_str(); }
	void SetPlaceId(const char *id) { placeId = id ? id : ""; }

	int GetPlaceVersion() const { return placeVersion; }
	void SetPlaceVersion(int version) { placeVersion = version; }

	// Quick access to common services
	std::shared_ptr<Workspace> GetWorkspace();
	std::shared_ptr<RunService> GetRunService();

	// Bind the workspace if created externally
	void SetWorkspace(std::shared_ptr<Workspace> ws);

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();

		// Properties (read-only for scripts)
		ClassDB::BindPropertyReadOnly<T, "GameId", "Data",
				&DataModel::GetGameId, NoneSecurity, ThreadSafety::Safe, true>({});

		ClassDB::BindPropertyReadOnly<T, "PlaceId", "Data",
				&DataModel::GetPlaceId, NoneSecurity, ThreadSafety::Safe, true>({});

		ClassDB::BindPropertyReadOnly<T, "PlaceVersion", "Data",
				&DataModel::GetPlaceVersion, NoneSecurity, ThreadSafety::Safe, true>({});

		// Methods
		ClassDB::BindLuauMethod<T, "GetService", std::shared_ptr<Instance>(const char *),
				&T::GetServiceLuau, NoneSecurity, ThreadSafety::Safe>({}, "className");

		ClassDB::BindLuauMethod<T, "FindService", std::shared_ptr<Instance>(const char *),
				&T::FindServiceLuau, NoneSecurity, ThreadSafety::Safe>({}, "className");

		// Workspace property - accessor to Workspace service
		LuauClassBinder<T>::AddIndexOverride(WorkspaceIndexOverride);
	}

private:
	std::string gameId;
	std::string placeId;
	int placeVersion = 0;

	// Service cache
	std::unordered_map<std::string, std::shared_ptr<Instance>> services;
	std::weak_ptr<Workspace> workspace;

	// Custom index override for Workspace property
	static int WorkspaceIndexOverride(lua_State *L, const char *propName);
};

} // namespace SBX::Classes
