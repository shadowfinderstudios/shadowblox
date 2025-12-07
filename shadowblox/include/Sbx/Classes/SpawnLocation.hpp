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

#include "Sbx/Classes/Part.hpp"

namespace SBX::Classes {

/**
 * @brief This class implements Roblox's [`SpawnLocation`](https://create.roblox.com/docs/reference/engine/classes/SpawnLocation)
 * class.
 *
 * SpawnLocation is a Part where players can spawn. It has properties to control
 * whether it's enabled and which team can spawn there.
 */
class SpawnLocation : public Part {
	SBXCLASS(SpawnLocation, Part, MemoryCategory::Instances);

public:
	SpawnLocation();
	~SpawnLocation() override = default;

	// Whether players can spawn here
	bool GetEnabled() const { return enabled; }
	void SetEnabled(bool value);

	// Whether this spawn is neutral (all teams can use)
	bool GetNeutral() const { return neutral; }
	void SetNeutral(bool value);

	// Team color (simplified as string)
	const char *GetTeamColor() const { return teamColor.c_str(); }
	void SetTeamColor(const char *color);

	// Spawn duration (how long to stay invulnerable)
	double GetDuration() const { return duration; }
	void SetDuration(double value);

	// Allow team change on touch
	bool GetAllowTeamChangeOnTouch() const { return allowTeamChangeOnTouch; }
	void SetAllowTeamChangeOnTouch(bool value);

protected:
	template <typename T>
	static void BindMembers() {
		Part::BindMembers<T>();

		ClassDB::BindProperty<T, "Enabled", "SpawnLocation", &SpawnLocation::GetEnabled, NoneSecurity,
				&SpawnLocation::SetEnabled, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "Neutral", "SpawnLocation", &SpawnLocation::GetNeutral, NoneSecurity,
				&SpawnLocation::SetNeutral, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "TeamColor", "SpawnLocation", &SpawnLocation::GetTeamColor, NoneSecurity,
				&SpawnLocation::SetTeamColor, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "Duration", "SpawnLocation", &SpawnLocation::GetDuration, NoneSecurity,
				&SpawnLocation::SetDuration, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "AllowTeamChangeOnTouch", "SpawnLocation", &SpawnLocation::GetAllowTeamChangeOnTouch, NoneSecurity,
				&SpawnLocation::SetAllowTeamChangeOnTouch, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
	}

private:
	bool enabled = true;
	bool neutral = false;
	std::string teamColor = "White";
	double duration = 10.0;
	bool allowTeamChangeOnTouch = false;
};

} // namespace SBX::Classes
