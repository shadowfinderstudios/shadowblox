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

#include "Sbx/GodotBridge.hpp"

#include "Sbx/Classes/ClassDB.hpp"
#include "Sbx/Classes/Instance.hpp"
#include "Sbx/Classes/Model.hpp"
#include "Sbx/Classes/Object.hpp"
#include "Sbx/Classes/Part.hpp"
#include "Sbx/DataTypes/Types.hpp"
#include "Sbx/DataTypes/Vector3.hpp"

namespace SBX::Bridge {

void InitializeAllClasses() {
	Classes::Object::InitializeClass();
	Classes::Instance::InitializeClass();
	Classes::Part::InitializeClass();
	Classes::Model::InitializeClass();
}

void RegisterAllClasses(lua_State *L) {
	DataTypes::luaSBX_opendatatypes(L);
	Classes::ClassDB::Register(L);
}

// Part functions
std::shared_ptr<Classes::Part> CreatePart() {
	auto part = std::make_shared<Classes::Part>();
	part->SetSelf(part);
	return part;
}

const char *Part_GetName(Classes::Part *part) {
	if (!part) return "";
	return part->GetName();
}

void Part_SetName(Classes::Part *part, const char *name) {
	if (part) part->SetName(name);
}

void Part_GetSize(Classes::Part *part, double *x, double *y, double *z) {
	if (!part) {
		*x = *y = *z = 0.0;
		return;
	}
	DataTypes::Vector3 size = part->GetSize();
	*x = size.X;
	*y = size.Y;
	*z = size.Z;
}

void Part_SetSize(Classes::Part *part, double x, double y, double z) {
	if (part) part->SetSize(DataTypes::Vector3(x, y, z));
}

void Part_GetPosition(Classes::Part *part, double *x, double *y, double *z) {
	if (!part) {
		*x = *y = *z = 0.0;
		return;
	}
	DataTypes::Vector3 pos = part->GetPosition();
	*x = pos.X;
	*y = pos.Y;
	*z = pos.Z;
}

void Part_SetPosition(Classes::Part *part, double x, double y, double z) {
	if (part) part->SetPosition(DataTypes::Vector3(x, y, z));
}

bool Part_GetAnchored(Classes::Part *part) {
	if (!part) return false;
	return part->GetAnchored();
}

void Part_SetAnchored(Classes::Part *part, bool anchored) {
	if (part) part->SetAnchored(anchored);
}

bool Part_GetCanCollide(Classes::Part *part) {
	if (!part) return true;
	return part->GetCanCollide();
}

void Part_SetCanCollide(Classes::Part *part, bool canCollide) {
	if (part) part->SetCanCollide(canCollide);
}

double Part_GetTransparency(Classes::Part *part) {
	if (!part) return 0.0;
	return part->GetTransparency();
}

void Part_SetTransparency(Classes::Part *part, double transparency) {
	if (part) part->SetTransparency(transparency);
}

bool Part_GetCanTouch(Classes::Part *part) {
	if (!part) return true;
	return part->GetCanTouch();
}

void Part_SetCanTouch(Classes::Part *part, bool canTouch) {
	if (part) part->SetCanTouch(canTouch);
}

// Model functions
std::shared_ptr<Classes::Model> CreateModel() {
	auto model = std::make_shared<Classes::Model>();
	model->SetSelf(model);
	return model;
}

const char *Model_GetName(Classes::Model *model) {
	if (!model) return "";
	return model->GetName();
}

void Model_SetName(Classes::Model *model, const char *name) {
	if (model) model->SetName(name);
}

std::shared_ptr<Classes::Part> Model_GetPrimaryPart(Classes::Model *model) {
	if (!model) return nullptr;
	return model->GetPrimaryPart();
}

void Model_SetPrimaryPart(Classes::Model *model, std::shared_ptr<Classes::Part> part) {
	if (model) model->SetPrimaryPart(part);
}

void Model_GetExtentsSize(Classes::Model *model, double *x, double *y, double *z) {
	if (!model) {
		*x = *y = *z = 0.0;
		return;
	}
	DataTypes::Vector3 size = model->GetExtentsSize();
	*x = size.X;
	*y = size.Y;
	*z = size.Z;
}

void Model_MoveTo(Classes::Model *model, double x, double y, double z) {
	if (model) model->MoveTo(DataTypes::Vector3(x, y, z));
}

void Model_TranslateBy(Classes::Model *model, double x, double y, double z) {
	if (model) model->TranslateBy(DataTypes::Vector3(x, y, z));
}

// Instance functions
void Instance_SetParent(Classes::Instance *child, std::shared_ptr<Classes::Instance> parent) {
	if (child) child->SetParent(parent);
}

std::shared_ptr<Classes::Instance> Instance_GetParent(Classes::Instance *instance) {
	if (!instance) return nullptr;
	return instance->GetParent();
}

} // namespace SBX::Bridge
