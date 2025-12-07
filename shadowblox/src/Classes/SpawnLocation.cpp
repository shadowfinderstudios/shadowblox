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

#include "Sbx/Classes/SpawnLocation.hpp"

namespace SBX::Classes {

SpawnLocation::SpawnLocation() :
		Part() {
	SetName("SpawnLocation");
	// SpawnLocations are typically anchored and have collisions
	SetAnchored(true);
	SetCanCollide(true);
}

void SpawnLocation::SetEnabled(bool value) {
	enabled = value;
	Changed<SpawnLocation>("Enabled");
}

void SpawnLocation::SetNeutral(bool value) {
	neutral = value;
	Changed<SpawnLocation>("Neutral");
}

void SpawnLocation::SetTeamColor(const char *color) {
	teamColor = color ? color : "";
	Changed<SpawnLocation>("TeamColor");
}

void SpawnLocation::SetDuration(double value) {
	duration = value;
	if (duration < 0) {
		duration = 0;
	}
	Changed<SpawnLocation>("Duration");
}

void SpawnLocation::SetAllowTeamChangeOnTouch(bool value) {
	allowTeamChangeOnTouch = value;
	Changed<SpawnLocation>("AllowTeamChangeOnTouch");
}

} // namespace SBX::Classes
