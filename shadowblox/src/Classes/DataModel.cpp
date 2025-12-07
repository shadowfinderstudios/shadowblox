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

#include "Sbx/Classes/DataModel.hpp"

#include <cstring>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Classes/ClassDB.hpp"
#include "Sbx/Classes/Players.hpp"
#include "Sbx/Classes/RunService.hpp"
#include "Sbx/Classes/Workspace.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::Classes {

DataModel::DataModel() :
		Instance() {
	SetName("Game");
}

DataModel::~DataModel() {
}

std::shared_ptr<Instance> DataModel::GetService(const char *className) {
	if (!className) {
		return nullptr;
	}

	// Check if service already exists
	auto it = services.find(className);
	if (it != services.end()) {
		return it->second;
	}

	// Check if the class is a service
	const ClassDB::ClassInfo *classInfo = ClassDB::GetClass(className);
	if (!classInfo) {
		return nullptr;
	}

	if (!classInfo->tags.contains(ClassTag::Service)) {
		return nullptr;
	}

	// Create the service
	std::shared_ptr<Instance> service;

	// Handle known service types
	if (std::strcmp(className, "Workspace") == 0) {
		auto ws = std::make_shared<Workspace>();
		ws->SetSelf(ws);
		ws->SetParent(GetSelf());
		workspace = ws;
		service = ws;
	} else if (std::strcmp(className, "RunService") == 0) {
		auto rs = std::make_shared<RunService>();
		rs->SetSelf(rs);
		rs->SetParent(GetSelf());
		service = rs;
	} else if (std::strcmp(className, "Players") == 0) {
		auto ps = std::make_shared<Players>();
		ps->SetSelf(ps);
		ps->SetParent(GetSelf());
		service = ps;
	} else {
		// Generic service creation would go here
		return nullptr;
	}

	services[className] = service;
	return service;
}

std::shared_ptr<Instance> DataModel::FindService(const char *className) const {
	if (!className) {
		return nullptr;
	}

	auto it = services.find(className);
	if (it != services.end()) {
		return it->second;
	}

	return nullptr;
}

std::shared_ptr<Workspace> DataModel::GetWorkspace() {
	auto ws = workspace.lock();
	if (!ws) {
		ws = std::dynamic_pointer_cast<Workspace>(GetService("Workspace"));
	}
	return ws;
}

std::shared_ptr<RunService> DataModel::GetRunService() {
	auto service = FindService("RunService");
	if (!service) {
		service = GetService("RunService");
	}
	return std::dynamic_pointer_cast<RunService>(service);
}

void DataModel::SetWorkspace(std::shared_ptr<Workspace> ws) {
	if (ws) {
		workspace = ws;
		services["Workspace"] = ws;
		ws->SetParent(GetSelf());
	}
}

int DataModel::GetServiceLuau(lua_State *L) {
	DataModel *self = LuauStackOp<DataModel *>::Check(L, 1);
	const char *className = luaL_checkstring(L, 2);

	std::shared_ptr<Instance> service = self->GetService(className);
	if (service) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, service);
	} else {
		luaL_error(L, "'%s' is not a valid service name", className);
	}
	return 1;
}

int DataModel::FindServiceLuau(lua_State *L) {
	DataModel *self = LuauStackOp<DataModel *>::Check(L, 1);
	const char *className = luaL_checkstring(L, 2);

	std::shared_ptr<Instance> service = self->FindService(className);
	if (service) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, service);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int DataModel::WorkspaceIndexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "Workspace") == 0) {
		DataModel *self = LuauStackOp<DataModel *>::Check(L, 1);
		std::shared_ptr<Workspace> ws = self->GetWorkspace();
		if (ws) {
			LuauStackOp<std::shared_ptr<Instance>>::Push(L, ws);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}
	return 0;  // Not handled
}

} // namespace SBX::Classes
