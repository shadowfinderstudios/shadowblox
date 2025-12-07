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

#include "lua.h"

#include "Sbx/Classes/Instance.hpp"
#include "Sbx/DataTypes/Vector3.hpp"

namespace SBX::Classes {

/**
 * @brief This class implements a basic version of Roblox's [`Part`](https://create.roblox.com/docs/reference/engine/classes/Part)
 * class.
 *
 * Part is a basic building block for 3D worlds. It has size, position, and physical properties.
 * This is a simplified implementation focusing on core properties.
 */
class Part : public Instance {
	SBXCLASS(Part, Instance, MemoryCategory::Instances);

public:
	Part();
	~Part() override = default;

	// Size property
	DataTypes::Vector3 GetSize() const { return size; }
	void SetSize(DataTypes::Vector3 newSize);

	// Position property
	DataTypes::Vector3 GetPosition() const { return position; }
	void SetPosition(DataTypes::Vector3 newPosition);

	// Anchored property
	bool GetAnchored() const { return anchored; }
	void SetAnchored(bool value);

	// CanCollide property
	bool GetCanCollide() const { return canCollide; }
	void SetCanCollide(bool value);

	// Transparency property
	double GetTransparency() const { return transparency; }
	void SetTransparency(double value);

	// CanTouch property (for touch events)
	bool GetCanTouch() const { return canTouch; }
	void SetCanTouch(bool value);

	// Fire Touched/TouchEnded signals (called from engine when collision detected)
	void FireTouched(std::shared_ptr<Part> otherPart);
	void FireTouchEnded(std::shared_ptr<Part> otherPart);

	// Luau property accessors (for Vector3 properties that need special handling)
	static int GetSizeLuau(lua_State *L);
	static int SetSizeLuau(lua_State *L);
	static int GetPositionLuau(lua_State *L);
	static int SetPositionLuau(lua_State *L);

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();

		// Vector3 properties via custom index/newindex
		LuauClassBinder<T>::AddIndexOverride(PartIndexOverride);
		LuauClassBinder<T>::AddNewindexOverride(PartNewindexOverride);

		// Register Size and Position in ClassDB for reflection (not scriptable via normal path)
		ClassDB::BindPropertyNotScriptable<T, "Size", "Part",
				&Part::GetSize, &Part::SetSize,
				ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindPropertyNotScriptable<T, "Position", "Part",
				&Part::GetPosition, &Part::SetPosition,
				ThreadSafety::Unsafe, true, true>({});

		// Simple properties
		ClassDB::BindProperty<T, "Anchored", "Part", &Part::GetAnchored, NoneSecurity,
				&Part::SetAnchored, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "CanCollide", "Part", &Part::GetCanCollide, NoneSecurity,
				&Part::SetCanCollide, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "Transparency", "Part", &Part::GetTransparency, NoneSecurity,
				&Part::SetTransparency, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "CanTouch", "Part", &Part::GetCanTouch, NoneSecurity,
				&Part::SetCanTouch, NoneSecurity, ThreadSafety::Unsafe, true, true>({});

		// Signals
		ClassDB::BindSignal<T, "Touched", void(std::shared_ptr<Part>), NoneSecurity>({}, "otherPart");
		ClassDB::BindSignal<T, "TouchEnded", void(std::shared_ptr<Part>), NoneSecurity>({}, "otherPart");
	}

private:
	DataTypes::Vector3 size = DataTypes::Vector3(2.0, 1.0, 4.0);  // Default Roblox part size
	DataTypes::Vector3 position = DataTypes::Vector3::zero;
	bool anchored = false;
	bool canCollide = true;
	double transparency = 0.0;
	bool canTouch = true;

	// Custom index/newindex overrides for Vector3 properties
	static int PartIndexOverride(lua_State *L, const char *propName);
	static bool PartNewindexOverride(lua_State *L, const char *propName);
};

} //namespace SBX::Classes
