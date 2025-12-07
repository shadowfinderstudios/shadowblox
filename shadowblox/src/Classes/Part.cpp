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

#include "Sbx/Classes/Part.hpp"

#include <algorithm>
#include <cstring>

#include "lua.h"

#include "Sbx/Runtime/Stack.hpp"

namespace SBX::Classes {

Part::Part() :
		Instance() {
	SetName("Part");
}

void Part::SetSize(DataTypes::Vector3 newSize) {
	// Clamp size to reasonable values (Roblox minimum is 0.05)
	size = DataTypes::Vector3(
			std::max(0.05, newSize.X),
			std::max(0.05, newSize.Y),
			std::max(0.05, newSize.Z));
	Changed<Part>("Size");
}

void Part::SetPosition(DataTypes::Vector3 newPosition) {
	position = newPosition;
	Changed<Part>("Position");
}

void Part::SetAnchored(bool value) {
	anchored = value;
	Changed<Part>("Anchored");
}

void Part::SetCanCollide(bool value) {
	canCollide = value;
	Changed<Part>("CanCollide");
}

void Part::SetTransparency(double value) {
	// Clamp transparency to [0, 1]
	transparency = std::max(0.0, std::min(1.0, value));
	Changed<Part>("Transparency");
}

void Part::SetCanTouch(bool value) {
	canTouch = value;
	Changed<Part>("CanTouch");
}

// Luau property accessors
int Part::GetSizeLuau(lua_State *L) {
	Part *self = LuauStackOp<Part *>::Check(L, 1);
	LuauStackOp<DataTypes::Vector3>::Push(L, self->GetSize());
	return 1;
}

int Part::SetSizeLuau(lua_State *L) {
	Part *self = LuauStackOp<Part *>::Check(L, 1);
	DataTypes::Vector3 newSize = LuauStackOp<DataTypes::Vector3>::Check(L, 3);
	self->SetSize(newSize);
	return 0;
}

int Part::GetPositionLuau(lua_State *L) {
	Part *self = LuauStackOp<Part *>::Check(L, 1);
	LuauStackOp<DataTypes::Vector3>::Push(L, self->GetPosition());
	return 1;
}

int Part::SetPositionLuau(lua_State *L) {
	Part *self = LuauStackOp<Part *>::Check(L, 1);
	DataTypes::Vector3 newPosition = LuauStackOp<DataTypes::Vector3>::Check(L, 3);
	self->SetPosition(newPosition);
	return 0;
}

// Custom index/newindex overrides for Vector3 properties
int Part::PartIndexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "Size") == 0) {
		return GetSizeLuau(L);
	}
	if (std::strcmp(propName, "Position") == 0) {
		return GetPositionLuau(L);
	}
	return 0; // Not handled
}

bool Part::PartNewindexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "Size") == 0) {
		SetSizeLuau(L);
		return true;
	}
	if (std::strcmp(propName, "Position") == 0) {
		SetPositionLuau(L);
		return true;
	}
	return false; // Not handled
}

} //namespace SBX::Classes
