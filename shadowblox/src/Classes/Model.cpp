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

#include "Sbx/Classes/Model.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

#include "lua.h"

#include "Sbx/Classes/Part.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::Classes {

Model::Model() :
		Instance() {
	SetName("Model");
}

std::shared_ptr<Part> Model::GetPrimaryPart() const {
	return primaryPart.lock();
}

void Model::SetPrimaryPart(std::shared_ptr<Part> part) {
	// Verify that the part is a descendant of this model (or nullptr)
	if (part && !IsAncestorOf(part.get())) {
		return; // PrimaryPart must be a descendant
	}
	primaryPart = part;
	Changed<Model>("PrimaryPart");
}

void Model::CollectParts(std::vector<Part *> &parts) const {
	for (const auto &child : GetChildren()) {
		Part *part = dynamic_cast<Part *>(child.get());
		if (part) {
			parts.push_back(part);
		}

		// Recurse into children
		Model *model = dynamic_cast<Model *>(child.get());
		if (model) {
			model->CollectParts(parts);
		} else if (child) {
			// Check descendants of non-Model Instances
			for (const auto &descendant : child->GetDescendants()) {
				Part *descPart = dynamic_cast<Part *>(descendant.get());
				if (descPart) {
					parts.push_back(descPart);
				}
			}
		}
	}
}

std::pair<DataTypes::Vector3, DataTypes::Vector3> Model::GetBoundingBox() const {
	std::vector<Part *> parts;
	CollectParts(parts);

	if (parts.empty()) {
		return { DataTypes::Vector3::zero, DataTypes::Vector3::zero };
	}

	double minX = std::numeric_limits<double>::max();
	double minY = std::numeric_limits<double>::max();
	double minZ = std::numeric_limits<double>::max();
	double maxX = std::numeric_limits<double>::lowest();
	double maxY = std::numeric_limits<double>::lowest();
	double maxZ = std::numeric_limits<double>::lowest();

	for (Part *part : parts) {
		DataTypes::Vector3 pos = part->GetPosition();
		DataTypes::Vector3 size = part->GetSize();
		DataTypes::Vector3 halfSize = size * 0.5;

		minX = std::min(minX, pos.X - halfSize.X);
		minY = std::min(minY, pos.Y - halfSize.Y);
		minZ = std::min(minZ, pos.Z - halfSize.Z);
		maxX = std::max(maxX, pos.X + halfSize.X);
		maxY = std::max(maxY, pos.Y + halfSize.Y);
		maxZ = std::max(maxZ, pos.Z + halfSize.Z);
	}

	return {
		DataTypes::Vector3(minX, minY, minZ),
		DataTypes::Vector3(maxX, maxY, maxZ)
	};
}

DataTypes::Vector3 Model::GetExtentsSize() const {
	auto [min, max] = GetBoundingBox();
	return max - min;
}

void Model::MoveTo(DataTypes::Vector3 position) {
	std::shared_ptr<Part> primary = primaryPart.lock();
	if (!primary) {
		return; // Can't move without PrimaryPart
	}

	DataTypes::Vector3 offset = position - primary->GetPosition();
	TranslateBy(offset);
}

void Model::TranslateBy(DataTypes::Vector3 offset) {
	std::vector<Part *> parts;
	CollectParts(parts);

	for (Part *part : parts) {
		part->SetPosition(part->GetPosition() + offset);
	}
}

// Luau method implementations
int Model::GetPrimaryPartLuau(lua_State *L) {
	Model *self = LuauStackOp<Model *>::Check(L, 1);
	std::shared_ptr<Part> primary = self->GetPrimaryPart();
	if (primary) {
		LuauStackOp<std::shared_ptr<Part>>::Push(L, primary);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Model::SetPrimaryPartLuau(lua_State *L) {
	Model *self = LuauStackOp<Model *>::Check(L, 1);
	std::shared_ptr<Part> part;
	if (!lua_isnil(L, 3)) {
		part = LuauStackOp<std::shared_ptr<Part>>::Check(L, 3);
	}
	self->SetPrimaryPart(part);
	return 0;
}

int Model::GetExtentsSizeLuau(lua_State *L) {
	Model *self = LuauStackOp<Model *>::Check(L, 1);
	LuauStackOp<DataTypes::Vector3>::Push(L, self->GetExtentsSize());
	return 1;
}

int Model::MoveToLuau(lua_State *L) {
	Model *self = LuauStackOp<Model *>::Check(L, 1);
	DataTypes::Vector3 position = LuauStackOp<DataTypes::Vector3>::Check(L, 2);
	self->MoveTo(position);
	return 0;
}

int Model::TranslateByLuau(lua_State *L) {
	Model *self = LuauStackOp<Model *>::Check(L, 1);
	DataTypes::Vector3 offset = LuauStackOp<DataTypes::Vector3>::Check(L, 2);
	self->TranslateBy(offset);
	return 0;
}

// Custom index/newindex overrides for PrimaryPart property
int Model::ModelIndexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "PrimaryPart") == 0) {
		return GetPrimaryPartLuau(L);
	}
	return 0; // Not handled
}

bool Model::ModelNewindexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "PrimaryPart") == 0) {
		SetPrimaryPartLuau(L);
		return true;
	}
	return false; // Not handled
}

} //namespace SBX::Classes
