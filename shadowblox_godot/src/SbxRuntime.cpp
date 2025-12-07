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

#include "SbxGD/SbxRuntime.hpp"
#include "SbxGD/SbxPart.hpp"

#include <string>

#include "godot_cpp/variant/utility_functions.hpp"

#include "Luau/Compiler.h"
#include "lua.h"
#include "lualib.h"

#include "Sbx/Classes/DataModel.hpp"
#include "Sbx/Classes/Part.hpp"
#include "Sbx/Classes/Players.hpp"
#include "Sbx/Classes/Player.hpp"
#include "Sbx/Classes/RunService.hpp"
#include "Sbx/Classes/Script.hpp"
#include "Sbx/Classes/Workspace.hpp"
#include "Sbx/GodotBridge.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SbxGD {

// Singleton instance
SbxRuntime *SbxRuntime::singleton = nullptr;

static void runtime_init_callback(lua_State *L) {
	SBX::Bridge::RegisterAllClasses(L);
}

// GC step sizes for each VM type
static uint32_t gc_step_sizes[SBX::VMMax] = { 200, 200 };

SbxRuntime::SbxRuntime() {
	if (singleton == nullptr) {
		singleton = this;
	}
}

SbxRuntime::~SbxRuntime() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

void SbxRuntime::_ready() {
	initialize_runtime();
	setup_data_model();
}

void SbxRuntime::_process(double delta) {
	if (runtime) {
		// Step the garbage collector each frame
		runtime->GCStep(gc_step_sizes, delta);

		// Fire RunService signals
		elapsedTime += delta;
		fire_stepped(elapsedTime, delta);
		fire_heartbeat(delta);
	}
}

void SbxRuntime::initialize_runtime() {
	if (initialized) {
		return;
	}

	// Initialize shadowblox classes via bridge
	SBX::Bridge::InitializeAllClasses();

	// Create the Luau runtime
	runtime = std::make_unique<SBX::LuauRuntime>(runtime_init_callback);

	initialized = true;
	godot::UtilityFunctions::print("[SbxRuntime] Luau runtime initialized");
}

void SbxRuntime::setup_data_model() {
	if (!runtime) return;

	// Create DataModel (the 'game' object)
	dataModel = SBX::Bridge::CreateDataModel();

	// Get the VM and register globals
	lua_State *L = runtime->GetVM(SBX::UserVM);
	SBX::Bridge::RegisterGlobals(L, dataModel);

	godot::UtilityFunctions::print("[SbxRuntime] DataModel created - game, workspace, Players available");
}

std::shared_ptr<SBX::Classes::Workspace> SbxRuntime::get_workspace() const {
	if (!dataModel) return nullptr;
	return dataModel->GetWorkspace();
}

std::shared_ptr<SBX::Classes::Players> SbxRuntime::get_players() const {
	if (!dataModel) return nullptr;
	return std::dynamic_pointer_cast<SBX::Classes::Players>(dataModel->GetService("Players"));
}

std::shared_ptr<SBX::Classes::RunService> SbxRuntime::get_run_service() const {
	if (!dataModel) return nullptr;
	return dataModel->GetRunService();
}

godot::String SbxRuntime::execute_script(const godot::String &code) {
	if (!runtime) {
		return "Error: Runtime not initialized";
	}

	lua_State *L = runtime->GetVM(SBX::UserVM);
	lua_State *T = lua_newthread(L);
	luaL_sandboxthread(T);

	// Compile the script
	Luau::CompileOptions opts;
	std::string bytecode = Luau::compile(code.utf8().get_data(), opts);

	// Load and execute
	if (luau_load(T, "=script", bytecode.data(), bytecode.size(), 0) != 0) {
		std::string error = SBX::LuauStackOp<std::string>::Get(T, -1);
		lua_pop(T, 1);
		return godot::String("Compile Error: ") + godot::String(error.c_str());
	}

	int status = SBX::luaSBX_resume(T, nullptr, 0, 1.0);

	if (status != LUA_OK && status != LUA_YIELD) {
		std::string error = SBX::LuauStackOp<std::string>::Get(T, -1);
		lua_pop(T, 1);
		return godot::String("Runtime Error: ") + godot::String(error.c_str());
	}

	return "OK";
}

godot::String SbxRuntime::run_script(const godot::String &code, SbxPart *script_parent) {
	if (!runtime) {
		return "Error: Runtime not initialized";
	}

	lua_State *L = runtime->GetVM(SBX::UserVM);
	lua_State *T = lua_newthread(L);
	luaL_sandboxthread(T);

	// Create a Script object and set it as global
	auto script = std::make_shared<SBX::Classes::Script>();
	script->SetSelf(script);
	script->SetSource(code.utf8().get_data());

	// Set script parent if provided
	if (script_parent && script_parent->get_sbx_part()) {
		script->SetParent(script_parent->get_sbx_part());
	}

	// Register 'script' global in this thread
	SBX::Bridge::RegisterScriptGlobal(T, script);

	// Compile the script
	Luau::CompileOptions opts;
	std::string bytecode = Luau::compile(code.utf8().get_data(), opts);

	// Load and execute
	if (luau_load(T, "=script", bytecode.data(), bytecode.size(), 0) != 0) {
		std::string error = SBX::LuauStackOp<std::string>::Get(T, -1);
		lua_pop(T, 1);
		return godot::String("Compile Error: ") + godot::String(error.c_str());
	}

	int status = SBX::luaSBX_resume(T, nullptr, 0, 1.0);

	if (status != LUA_OK && status != LUA_YIELD) {
		std::string error = SBX::LuauStackOp<std::string>::Get(T, -1);
		lua_pop(T, 1);
		return godot::String("Runtime Error: ") + godot::String(error.c_str());
	}

	return "OK";
}

void SbxRuntime::create_local_player(int64_t user_id, const godot::String &display_name) {
	auto players = get_players();
	if (players) {
		players->CreateLocalPlayer(user_id, display_name.utf8().get_data());
		godot::UtilityFunctions::print("[SbxRuntime] Created local player: ", display_name);
	}
}

void SbxRuntime::fire_heartbeat(double delta) {
	auto runService = get_run_service();
	if (runService) {
		runService->FireHeartbeat(delta);
	}
}

void SbxRuntime::fire_stepped(double time, double delta) {
	auto runService = get_run_service();
	if (runService) {
		runService->FireStepped(time, delta);
	}
}

void SbxRuntime::gc_step(int step_size) {
	if (runtime) {
		uint32_t steps[SBX::VMMax] = { static_cast<uint32_t>(step_size), static_cast<uint32_t>(step_size) };
		runtime->GCStep(steps, 0.0);
	}
}

int SbxRuntime::get_gc_memory() const {
	if (!runtime) {
		return 0;
	}

	int32_t memory[SBX::VMMax];
	runtime->GCSize(memory);
	return static_cast<int>(memory[0] + memory[1]);
}

void SbxRuntime::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("execute_script", "code"), &SbxRuntime::execute_script);
	godot::ClassDB::bind_method(godot::D_METHOD("gc_step", "step_size"), &SbxRuntime::gc_step);
	godot::ClassDB::bind_method(godot::D_METHOD("get_gc_memory"), &SbxRuntime::get_gc_memory);
	godot::ClassDB::bind_method(godot::D_METHOD("create_local_player", "user_id", "display_name"), &SbxRuntime::create_local_player);
	godot::ClassDB::bind_method(godot::D_METHOD("fire_heartbeat", "delta"), &SbxRuntime::fire_heartbeat);
	godot::ClassDB::bind_method(godot::D_METHOD("fire_stepped", "time", "delta"), &SbxRuntime::fire_stepped);
}

} // namespace SbxGD
