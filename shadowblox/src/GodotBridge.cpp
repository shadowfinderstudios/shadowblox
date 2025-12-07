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

#include "lua.h"

#include "Sbx/Classes/ClassDB.hpp"
#include "Sbx/Classes/DataModel.hpp"
#include "Sbx/Classes/Instance.hpp"
#include "Sbx/Classes/Model.hpp"
#include "Sbx/Classes/Object.hpp"
#include "Sbx/Classes/Part.hpp"
#include "Sbx/Classes/RunService.hpp"
#include "Sbx/Classes/Workspace.hpp"
#include "Sbx/DataTypes/Types.hpp"
#include "Sbx/DataTypes/Vector3.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::Bridge {

void InitializeAllClasses() {
	Classes::Object::InitializeClass();
	Classes::Instance::InitializeClass();
	Classes::Part::InitializeClass();
	Classes::Model::InitializeClass();
	Classes::DataModel::InitializeClass();
	Classes::Workspace::InitializeClass();
	Classes::RunService::InitializeClass();
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

// DataModel functions
std::shared_ptr<Classes::DataModel> CreateDataModel() {
	auto dataModel = std::make_shared<Classes::DataModel>();
	dataModel->SetSelf(dataModel);
	return dataModel;
}

std::shared_ptr<Classes::Workspace> DataModel_GetWorkspace(Classes::DataModel *dataModel) {
	if (!dataModel) return nullptr;
	return dataModel->GetWorkspace();
}

std::shared_ptr<Classes::RunService> DataModel_GetRunService(Classes::DataModel *dataModel) {
	if (!dataModel) return nullptr;
	return dataModel->GetRunService();
}

std::shared_ptr<Classes::Instance> DataModel_GetService(Classes::DataModel *dataModel, const char *serviceName) {
	if (!dataModel) return nullptr;
	return dataModel->GetService(serviceName);
}

// Workspace functions
std::shared_ptr<Classes::Workspace> CreateWorkspace() {
	auto workspace = std::make_shared<Classes::Workspace>();
	workspace->SetSelf(workspace);
	return workspace;
}

void Workspace_GetGravity(Classes::Workspace *workspace, double *x, double *y, double *z) {
	if (!workspace) {
		*x = 0.0;
		*y = -196.2;
		*z = 0.0;
		return;
	}
	DataTypes::Vector3 gravity = workspace->GetGravity();
	*x = gravity.X;
	*y = gravity.Y;
	*z = gravity.Z;
}

void Workspace_SetGravity(Classes::Workspace *workspace, double x, double y, double z) {
	if (workspace) workspace->SetGravity(DataTypes::Vector3(x, y, z));
}

// RunService functions
std::shared_ptr<Classes::RunService> CreateRunService() {
	auto runService = std::make_shared<Classes::RunService>();
	runService->SetSelf(runService);
	return runService;
}

void RunService_FireStepped(Classes::RunService *runService, double time, double deltaTime) {
	if (runService) runService->FireStepped(time, deltaTime);
}

void RunService_FireHeartbeat(Classes::RunService *runService, double deltaTime) {
	if (runService) runService->FireHeartbeat(deltaTime);
}

void RunService_FireRenderStepped(Classes::RunService *runService, double deltaTime) {
	if (runService) runService->FireRenderStepped(deltaTime);
}

void RunService_Run(Classes::RunService *runService) {
	if (runService) runService->Run();
}

void RunService_Pause(Classes::RunService *runService) {
	if (runService) runService->Pause();
}

void RunService_Stop(Classes::RunService *runService) {
	if (runService) runService->Stop();
}

bool RunService_IsRunning(Classes::RunService *runService) {
	if (!runService) return false;
	return runService->IsRunning();
}

void RunService_SetIsClient(Classes::RunService *runService, bool isClient) {
	if (runService) runService->SetIsClient(isClient);
}

void RunService_SetIsServer(Classes::RunService *runService, bool isServer) {
	if (runService) runService->SetIsServer(isServer);
}

// Register game/workspace globals
void RegisterGlobals(lua_State *L, std::shared_ptr<Classes::DataModel> dataModel) {
	if (!L || !dataModel) return;

	// Ensure Workspace and RunService are created
	auto workspace = dataModel->GetWorkspace();
	dataModel->GetRunService();

	// Push game (DataModel) global
	LuauStackOp<std::shared_ptr<Classes::Instance>>::Push(L, dataModel);
	lua_setglobal(L, "game");

	// Also set as "Game" for compatibility
	LuauStackOp<std::shared_ptr<Classes::Instance>>::Push(L, dataModel);
	lua_setglobal(L, "Game");

	// Push workspace global
	if (workspace) {
		LuauStackOp<std::shared_ptr<Classes::Instance>>::Push(L, workspace);
		lua_setglobal(L, "workspace");

		// Also set as "Workspace" for compatibility
		LuauStackOp<std::shared_ptr<Classes::Instance>>::Push(L, workspace);
		lua_setglobal(L, "Workspace");
	}
}

} // namespace SBX::Bridge
