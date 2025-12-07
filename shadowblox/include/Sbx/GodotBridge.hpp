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

// Forward declarations only - avoid including template-heavy headers
struct lua_State;

namespace SBX {
class LuauRuntime;
}

namespace SBX::Classes {
class Instance;
class Part;
class Model;
}

namespace SBX::DataTypes {
class Vector3;
}

namespace SBX::Bridge {

// Runtime initialization
void InitializeAllClasses();
void RegisterAllClasses(lua_State *L);

// Part creation and access
std::shared_ptr<Classes::Part> CreatePart();
const char *Part_GetName(Classes::Part *part);
void Part_SetName(Classes::Part *part, const char *name);
void Part_GetSize(Classes::Part *part, double *x, double *y, double *z);
void Part_SetSize(Classes::Part *part, double x, double y, double z);
void Part_GetPosition(Classes::Part *part, double *x, double *y, double *z);
void Part_SetPosition(Classes::Part *part, double x, double y, double z);
bool Part_GetAnchored(Classes::Part *part);
void Part_SetAnchored(Classes::Part *part, bool anchored);
bool Part_GetCanCollide(Classes::Part *part);
void Part_SetCanCollide(Classes::Part *part, bool canCollide);
double Part_GetTransparency(Classes::Part *part);
void Part_SetTransparency(Classes::Part *part, double transparency);
bool Part_GetCanTouch(Classes::Part *part);
void Part_SetCanTouch(Classes::Part *part, bool canTouch);

// Model creation and access
std::shared_ptr<Classes::Model> CreateModel();
const char *Model_GetName(Classes::Model *model);
void Model_SetName(Classes::Model *model, const char *name);
std::shared_ptr<Classes::Part> Model_GetPrimaryPart(Classes::Model *model);
void Model_SetPrimaryPart(Classes::Model *model, std::shared_ptr<Classes::Part> part);
void Model_GetExtentsSize(Classes::Model *model, double *x, double *y, double *z);
void Model_MoveTo(Classes::Model *model, double x, double y, double z);
void Model_TranslateBy(Classes::Model *model, double x, double y, double z);

// Instance hierarchy
void Instance_SetParent(Classes::Instance *child, std::shared_ptr<Classes::Instance> parent);
std::shared_ptr<Classes::Instance> Instance_GetParent(Classes::Instance *instance);

} // namespace SBX::Bridge
