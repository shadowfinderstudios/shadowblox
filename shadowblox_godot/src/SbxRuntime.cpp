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

#include "godot_cpp/variant/utility_functions.hpp"

#include "Sbx/GodotBridge.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"

namespace SbxGD {

static void runtime_init_callback(lua_State *L) {
	SBX::Bridge::RegisterAllClasses(L);
}

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
		runtime->GCStep(200, delta);
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

	SBX::ExecOutput out = SBX::luaSBX_exec(T, code.utf8().get_data());

	if (out.status != LUA_OK) {
		return godot::String("Error: ") + godot::String(out.error.c_str());
	}

	return "OK";
}

void SbxRuntime::gc_step(int step_size) {
	if (runtime) {
		runtime->GCStep(step_size, 0.0);
	}
}

int SbxRuntime::get_gc_memory() const {
	if (!runtime) {
		return 0;
	}

	size_t memory[2];
	runtime->GCSize(memory);
	return static_cast<int>(memory[0] + memory[1]);
}

void SbxRuntime::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("execute_script", "code"), &SbxRuntime::execute_script);
	godot::ClassDB::bind_method(godot::D_METHOD("gc_step", "step_size"), &SbxRuntime::gc_step);
	godot::ClassDB::bind_method(godot::D_METHOD("get_gc_memory"), &SbxRuntime::get_gc_memory);
}

} // namespace SbxGD
