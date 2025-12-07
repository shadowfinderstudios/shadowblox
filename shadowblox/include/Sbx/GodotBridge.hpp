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
class DataModel;
class Workspace;
class RunService;
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

// DataModel creation and access
std::shared_ptr<Classes::DataModel> CreateDataModel();
std::shared_ptr<Classes::Workspace> DataModel_GetWorkspace(Classes::DataModel *dataModel);
std::shared_ptr<Classes::RunService> DataModel_GetRunService(Classes::DataModel *dataModel);
std::shared_ptr<Classes::Instance> DataModel_GetService(Classes::DataModel *dataModel, const char *serviceName);

// Workspace creation
std::shared_ptr<Classes::Workspace> CreateWorkspace();
void Workspace_GetGravity(Classes::Workspace *workspace, double *x, double *y, double *z);
void Workspace_SetGravity(Classes::Workspace *workspace, double x, double y, double z);

// RunService creation and control
std::shared_ptr<Classes::RunService> CreateRunService();
void RunService_FireStepped(Classes::RunService *runService, double time, double deltaTime);
void RunService_FireHeartbeat(Classes::RunService *runService, double deltaTime);
void RunService_FireRenderStepped(Classes::RunService *runService, double deltaTime);
void RunService_Run(Classes::RunService *runService);
void RunService_Pause(Classes::RunService *runService);
void RunService_Stop(Classes::RunService *runService);
bool RunService_IsRunning(Classes::RunService *runService);
void RunService_SetIsClient(Classes::RunService *runService, bool isClient);
void RunService_SetIsServer(Classes::RunService *runService, bool isServer);

// Register game/workspace globals in Lua state
void RegisterGlobals(lua_State *L, std::shared_ptr<Classes::DataModel> dataModel);

} // namespace SBX::Bridge
