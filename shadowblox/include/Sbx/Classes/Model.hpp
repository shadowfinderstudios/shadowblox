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
#include <utility>

#include "lua.h"

#include "Sbx/Classes/Instance.hpp"
#include "Sbx/DataTypes/Vector3.hpp"

namespace SBX::Classes {

// Forward declaration
class Part;

/**
 * @brief This class implements a basic version of Roblox's [`Model`](https://create.roblox.com/docs/reference/engine/classes/Model)
 * class.
 *
 * Model is a container for grouping Instances together. It has a PrimaryPart property
 * for physics and movement operations, and methods for calculating bounding boxes.
 */
class Model : public Instance {
	SBXCLASS(Model, Instance, MemoryCategory::Instances);

public:
	Model();
	~Model() override = default;

	// PrimaryPart property
	std::shared_ptr<Part> GetPrimaryPart() const;
	void SetPrimaryPart(std::shared_ptr<Part> part);

	// Methods
	// GetExtentsSize returns the size of the bounding box
	DataTypes::Vector3 GetExtentsSize() const;

	// GetBoundingBox returns min and max corners of the bounding box
	std::pair<DataTypes::Vector3, DataTypes::Vector3> GetBoundingBox() const;

	// MoveTo moves the model so PrimaryPart is at the specified position
	void MoveTo(DataTypes::Vector3 position);

	// TranslateBy moves the model by a relative offset
	void TranslateBy(DataTypes::Vector3 offset);

	// Luau method implementations
	static int GetPrimaryPartLuau(lua_State *L);
	static int SetPrimaryPartLuau(lua_State *L);
	static int GetExtentsSizeLuau(lua_State *L);
	static int MoveToLuau(lua_State *L);
	static int TranslateByLuau(lua_State *L);

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();

		// PrimaryPart via custom index/newindex
		LuauClassBinder<T>::AddIndexOverride(ModelIndexOverride);
		LuauClassBinder<T>::AddNewindexOverride(ModelNewindexOverride);

		// Register PrimaryPart in ClassDB for reflection
		ClassDB::BindPropertyNotScriptable<T, "PrimaryPart", "Data",
				&Model::GetPrimaryPart, &Model::SetPrimaryPart,
				ThreadSafety::Unsafe, true, true>({});

		// Methods
		ClassDB::BindLuauMethod<T, "GetExtentsSize", DataTypes::Vector3(),
				&T::GetExtentsSizeLuau, NoneSecurity, ThreadSafety::Safe>({});

		ClassDB::BindLuauMethod<T, "MoveTo", void(DataTypes::Vector3),
				&T::MoveToLuau, NoneSecurity, ThreadSafety::Unsafe>({}, "position");

		ClassDB::BindLuauMethod<T, "TranslateBy", void(DataTypes::Vector3),
				&T::TranslateByLuau, NoneSecurity, ThreadSafety::Unsafe>({}, "delta");
	}

private:
	std::weak_ptr<Part> primaryPart;

	// Helper to collect all Part descendants
	void CollectParts(std::vector<Part *> &parts) const;

	// Custom index/newindex overrides for PrimaryPart property
	static int ModelIndexOverride(lua_State *L, const char *propName);
	static bool ModelNewindexOverride(lua_State *L, const char *propName);
};

} //namespace SBX::Classes
