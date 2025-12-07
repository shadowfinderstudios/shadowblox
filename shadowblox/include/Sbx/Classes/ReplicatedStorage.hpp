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

#include "Sbx/Classes/Instance.hpp"

namespace SBX::Classes {

/**
 * @brief This class implements Roblox's [`ReplicatedStorage`](https://create.roblox.com/docs/reference/engine/classes/ReplicatedStorage)
 * service.
 *
 * ReplicatedStorage is a container service for objects that are replicated
 * to both the server and all connected clients. It is commonly used to store
 * RemoteEvents, RemoteFunctions, and other shared assets.
 */
class ReplicatedStorage : public Instance {
	SBXCLASS(ReplicatedStorage, Instance, MemoryCategory::Instances, ClassTag::NotCreatable, ClassTag::Service);

public:
	ReplicatedStorage();
	~ReplicatedStorage() override = default;

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();
		// ReplicatedStorage has no additional members beyond Instance
	}
};

} // namespace SBX::Classes
