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

#include "Sbx/Classes/Workspace.hpp"

#include "Sbx/Classes/ClassDB.hpp"

namespace SBX::Classes {

Workspace::Workspace() :
		Model() {
	SetName("Workspace");
}

Workspace::~Workspace() {
}

void Workspace::SetGravity(DataTypes::Vector3 value) {
	gravity = value;
	Changed<Workspace>("Gravity");
}

void Workspace::SetFallenPartsDestroyHeight(double value) {
	fallenPartsDestroyHeight = value;
	Changed<Workspace>("FallenPartsDestroyHeight");
}

void Workspace::SetStreamingEnabled(bool value) {
	streamingEnabled = value;
	Changed<Workspace>("StreamingEnabled");
}

void Workspace::SetStreamingMinRadius(double value) {
	if (value < 0) {
		value = 0;
	}
	streamingMinRadius = value;
	Changed<Workspace>("StreamingMinRadius");
}

void Workspace::SetStreamingTargetRadius(double value) {
	if (value < streamingMinRadius) {
		value = streamingMinRadius;
	}
	streamingTargetRadius = value;
	Changed<Workspace>("StreamingTargetRadius");
}

} // namespace SBX::Classes
