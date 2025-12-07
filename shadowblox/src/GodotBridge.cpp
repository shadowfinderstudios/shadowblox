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

#include <cstdio>

#include "lua.h"

#include "Sbx/Classes/ClassDB.hpp"
#include "Sbx/Classes/DataModel.hpp"
#include "Sbx/Classes/Humanoid.hpp"
#include "Sbx/Classes/Instance.hpp"
#include "Sbx/Classes/Model.hpp"
#include "Sbx/Classes/Object.hpp"
#include "Sbx/Classes/Part.hpp"
#include "Sbx/Classes/Player.hpp"
#include "Sbx/Classes/Players.hpp"
#include "Sbx/Classes/RemoteEvent.hpp"
#include "Sbx/Classes/RemoteFunction.hpp"
#include "Sbx/Classes/RunService.hpp"
#include "Sbx/Classes/Script.hpp"
#include "Sbx/Classes/SpawnLocation.hpp"
#include "Sbx/Classes/ValueBase.hpp"
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
	Classes::Player::InitializeClass();
	Classes::Players::InitializeClass();
	Classes::Script::InitializeClass();
	Classes::LocalScript::InitializeClass();
	Classes::ModuleScript::InitializeClass();
	Classes::Humanoid::InitializeClass();
	Classes::SpawnLocation::InitializeClass();
	Classes::RemoteEvent::InitializeClass();
	Classes::RemoteFunction::InitializeClass();
	Classes::ValueBase::InitializeClass();
	Classes::StringValue::InitializeClass();
	Classes::IntValue::InitializeClass();
	Classes::NumberValue::InitializeClass();
	Classes::BoolValue::InitializeClass();
	Classes::ObjectValue::InitializeClass();
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

// Players functions
std::shared_ptr<Classes::Players> DataModel_GetPlayers(Classes::DataModel *dataModel) {
	if (!dataModel) return nullptr;
	return std::dynamic_pointer_cast<Classes::Players>(dataModel->GetService("Players"));
}

std::shared_ptr<Classes::Player> Players_CreateLocalPlayer(Classes::Players *players, int64_t userId, const char *displayName) {
	if (!players) return nullptr;
	return players->CreateLocalPlayer(userId, displayName);
}

std::shared_ptr<Classes::Player> Players_GetLocalPlayer(Classes::Players *players) {
	if (!players) return nullptr;
	return players->GetLocalPlayer();
}

void Player_SetCharacter(Classes::Player *player, std::shared_ptr<Classes::Model> character) {
	if (player) player->SetCharacter(character);
}

std::shared_ptr<Classes::Model> Player_GetCharacter(Classes::Player *player) {
	if (!player) return nullptr;
	return player->GetCharacter();
}

// Script functions
std::shared_ptr<Classes::Script> CreateScript() {
	auto script = std::make_shared<Classes::Script>();
	script->SetSelf(script);
	return script;
}

void Script_SetSource(Classes::Script *script, const char *source) {
	if (script) script->SetSource(source);
}

const char *Script_GetSource(Classes::Script *script) {
	if (!script) return "";
	return script->GetSource();
}

void RegisterScriptGlobal(lua_State *L, std::shared_ptr<Classes::Script> script) {
	if (!L) return;
	if (script) {
		LuauStackOp<std::shared_ptr<Classes::Instance>>::Push(L, script);
	} else {
		lua_pushnil(L);
	}
	lua_setglobal(L, "script");
}

// Register game/workspace globals
void RegisterGlobals(lua_State *L, std::shared_ptr<Classes::DataModel> dataModel) {
	printf("[RegisterGlobals] Starting...\n");
	fflush(stdout);
	if (!L || !dataModel) {
		printf("[RegisterGlobals] ERROR: L or dataModel is null!\n");
		fflush(stdout);
		return;
	}

	printf("[RegisterGlobals] Getting workspace...\n");
	fflush(stdout);
	// Ensure services are created
	auto workspace = dataModel->GetWorkspace();
	printf("[RegisterGlobals] Got workspace, getting RunService...\n");
	fflush(stdout);
	dataModel->GetRunService();
	printf("[RegisterGlobals] Got RunService, getting Players...\n");
	fflush(stdout);
	dataModel->GetService("Players");
	printf("[RegisterGlobals] Got Players, skipping other services for now...\n");
	fflush(stdout);
	// Skip services that might not exist to isolate the crash
	// dataModel->GetService("ReplicatedStorage");
	// dataModel->GetService("ServerScriptService");
	// dataModel->GetService("ServerStorage");
	// dataModel->GetService("StarterGui");
	// dataModel->GetService("StarterPlayer");

	printf("[RegisterGlobals] About to push DataModel to Lua...\n");
	fflush(stdout);
	// Push game (DataModel) global
	LuauStackOp<std::shared_ptr<Classes::Instance>>::Push(L, dataModel);
	printf("[RegisterGlobals] DataModel pushed, setting global 'game'...\n");
	fflush(stdout);
	lua_setglobal(L, "game");
	printf("[RegisterGlobals] 'game' global set\n");
	fflush(stdout);

	// Also set as "Game" for compatibility
	LuauStackOp<std::shared_ptr<Classes::Instance>>::Push(L, dataModel);
	lua_setglobal(L, "Game");
	printf("[RegisterGlobals] 'Game' global set\n");
	fflush(stdout);

	// Push workspace global
	if (workspace) {
		printf("[RegisterGlobals] Pushing workspace to Lua...\n");
		fflush(stdout);
		LuauStackOp<std::shared_ptr<Classes::Instance>>::Push(L, workspace);
		lua_setglobal(L, "workspace");

		// Also set as "Workspace" for compatibility
		LuauStackOp<std::shared_ptr<Classes::Instance>>::Push(L, workspace);
		lua_setglobal(L, "Workspace");
		printf("[RegisterGlobals] Workspace globals set\n");
		fflush(stdout);
	}
	printf("[RegisterGlobals] Done!\n");
	fflush(stdout);
}

// Humanoid functions
std::shared_ptr<Classes::Humanoid> CreateHumanoid() {
	auto humanoid = std::make_shared<Classes::Humanoid>();
	humanoid->SetSelf(humanoid);
	return humanoid;
}

double Humanoid_GetHealth(Classes::Humanoid *humanoid) {
	if (!humanoid) return 0.0;
	return humanoid->GetHealth();
}

void Humanoid_SetHealth(Classes::Humanoid *humanoid, double health) {
	if (humanoid) humanoid->SetHealth(health);
}

double Humanoid_GetMaxHealth(Classes::Humanoid *humanoid) {
	if (!humanoid) return 100.0;
	return humanoid->GetMaxHealth();
}

void Humanoid_SetMaxHealth(Classes::Humanoid *humanoid, double maxHealth) {
	if (humanoid) humanoid->SetMaxHealth(maxHealth);
}

double Humanoid_GetWalkSpeed(Classes::Humanoid *humanoid) {
	if (!humanoid) return 16.0;
	return humanoid->GetWalkSpeed();
}

void Humanoid_SetWalkSpeed(Classes::Humanoid *humanoid, double walkSpeed) {
	if (humanoid) humanoid->SetWalkSpeed(walkSpeed);
}

void Humanoid_TakeDamage(Classes::Humanoid *humanoid, double amount) {
	if (humanoid) humanoid->TakeDamage(amount);
}

// SpawnLocation functions
std::shared_ptr<Classes::SpawnLocation> CreateSpawnLocation() {
	auto spawnLocation = std::make_shared<Classes::SpawnLocation>();
	spawnLocation->SetSelf(spawnLocation);
	return spawnLocation;
}

bool SpawnLocation_GetEnabled(Classes::SpawnLocation *spawnLocation) {
	if (!spawnLocation) return false;
	return spawnLocation->GetEnabled();
}

void SpawnLocation_SetEnabled(Classes::SpawnLocation *spawnLocation, bool enabled) {
	if (spawnLocation) spawnLocation->SetEnabled(enabled);
}

bool SpawnLocation_GetNeutral(Classes::SpawnLocation *spawnLocation) {
	if (!spawnLocation) return false;
	return spawnLocation->GetNeutral();
}

void SpawnLocation_SetNeutral(Classes::SpawnLocation *spawnLocation, bool neutral) {
	if (spawnLocation) spawnLocation->SetNeutral(neutral);
}

// RemoteEvent functions
std::shared_ptr<Classes::RemoteEvent> CreateRemoteEvent() {
	auto remoteEvent = std::make_shared<Classes::RemoteEvent>();
	remoteEvent->SetSelf(remoteEvent);
	return remoteEvent;
}

// RemoteFunction functions
std::shared_ptr<Classes::RemoteFunction> CreateRemoteFunction() {
	auto remoteFunction = std::make_shared<Classes::RemoteFunction>();
	remoteFunction->SetSelf(remoteFunction);
	return remoteFunction;
}

// Value classes
std::shared_ptr<Classes::StringValue> CreateStringValue() {
	auto stringValue = std::make_shared<Classes::StringValue>();
	stringValue->SetSelf(stringValue);
	return stringValue;
}

const char *StringValue_GetValue(Classes::StringValue *stringValue) {
	if (!stringValue) return "";
	return stringValue->GetValue();
}

void StringValue_SetValue(Classes::StringValue *stringValue, const char *value) {
	if (stringValue) stringValue->SetValue(value);
}

std::shared_ptr<Classes::IntValue> CreateIntValue() {
	auto intValue = std::make_shared<Classes::IntValue>();
	intValue->SetSelf(intValue);
	return intValue;
}

int64_t IntValue_GetValue(Classes::IntValue *intValue) {
	if (!intValue) return 0;
	return intValue->GetValue();
}

void IntValue_SetValue(Classes::IntValue *intValue, int64_t value) {
	if (intValue) intValue->SetValue(value);
}

std::shared_ptr<Classes::NumberValue> CreateNumberValue() {
	auto numberValue = std::make_shared<Classes::NumberValue>();
	numberValue->SetSelf(numberValue);
	return numberValue;
}

double NumberValue_GetValue(Classes::NumberValue *numberValue) {
	if (!numberValue) return 0.0;
	return numberValue->GetValue();
}

void NumberValue_SetValue(Classes::NumberValue *numberValue, double value) {
	if (numberValue) numberValue->SetValue(value);
}

std::shared_ptr<Classes::BoolValue> CreateBoolValue() {
	auto boolValue = std::make_shared<Classes::BoolValue>();
	boolValue->SetSelf(boolValue);
	return boolValue;
}

bool BoolValue_GetValue(Classes::BoolValue *boolValue) {
	if (!boolValue) return false;
	return boolValue->GetValue();
}

void BoolValue_SetValue(Classes::BoolValue *boolValue, bool value) {
	if (boolValue) boolValue->SetValue(value);
}

std::shared_ptr<Classes::ObjectValue> CreateObjectValue() {
	auto objectValue = std::make_shared<Classes::ObjectValue>();
	objectValue->SetSelf(objectValue);
	return objectValue;
}

// Network callback handling
static NetworkEventCallback g_networkEventCallback = nullptr;
static NetworkFunctionCallback g_networkFunctionCallback = nullptr;

void SetNetworkEventCallback(NetworkEventCallback callback) {
	g_networkEventCallback = callback;

	// Also set the callback on RemoteEvent class
	if (callback) {
		Classes::RemoteEvent::SetNetworkCallback([](const char *eventName, int64_t targetId, const std::vector<uint8_t> &data) {
			if (g_networkEventCallback) {
				g_networkEventCallback(eventName, targetId, data.data(), data.size());
			}
		});
	} else {
		Classes::RemoteEvent::SetNetworkCallback(nullptr);
	}
}

void SetNetworkFunctionCallback(NetworkFunctionCallback callback) {
	g_networkFunctionCallback = callback;

	// Also set the callback on RemoteFunction class
	if (callback) {
		Classes::RemoteFunction::SetNetworkCallback([](const char *functionName, int64_t targetId, const std::vector<uint8_t> &data) -> std::vector<uint8_t> {
			if (g_networkFunctionCallback) {
				uint8_t *response = nullptr;
				size_t responseSize = 0;
				g_networkFunctionCallback(functionName, targetId, data.data(), data.size(), &response, &responseSize);
				if (response && responseSize > 0) {
					std::vector<uint8_t> result(response, response + responseSize);
					// Caller is responsible for freeing response
					return result;
				}
			}
			return {};
		});
	} else {
		Classes::RemoteFunction::SetNetworkCallback(nullptr);
	}
}

void ProcessNetworkEvent(const char *eventName, int64_t senderId, const uint8_t *data, size_t dataSize, lua_State *L, std::shared_ptr<Classes::Player> sender) {
	// This function processes an incoming network event
	// Find the RemoteEvent by name and trigger the appropriate signal
	// For now, this is a placeholder - the actual implementation would search the DataModel
}

} // namespace SBX::Bridge
