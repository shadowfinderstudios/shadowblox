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

#include <string>

#include "godot_cpp/variant/utility_functions.hpp"

#include "Luau/Compiler.h"
#include "lua.h"
#include "lualib.h"

#include "Sbx/GodotBridge.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SbxGD {

static void runtime_init_callback(lua_State *L) {
	SBX::Bridge::RegisterAllClasses(L);
}

// GC step sizes for each VM type
static uint32_t gc_step_sizes[SBX::VMMax] = { 200, 200 };

SbxRuntime::SbxRuntime() {
}

SbxRuntime::~SbxRuntime() {
}

void SbxRuntime::_ready() {
	initialize_runtime();
}

void SbxRuntime::_process(double delta) {
	if (runtime) {
		// Step the garbage collector each frame
		runtime->GCStep(gc_step_sizes, delta);
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
	godot::UtilityFunctions::print("[SbxRuntime] Initialized");
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
}

} // namespace SbxGD
