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

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/core/class_db.hpp"

namespace SBX {
class LuauRuntime;
}

namespace SBX::Classes {
class DataModel;
class Workspace;
class Players;
class Player;
class RunService;
class Script;
class Instance;
}

namespace SbxGD {

class SbxPart;

// SbxRuntime - Manages the Luau runtime within Godot
// Add as a child of your scene to enable script execution
class SbxRuntime : public godot::Node {
	GDCLASS(SbxRuntime, godot::Node);

public:
	SbxRuntime();
	~SbxRuntime();

	// Godot lifecycle
	void _ready() override;
	void _process(double delta) override;

	// Runtime access
	SBX::LuauRuntime *get_runtime() const { return runtime.get(); }

	// DataModel access
	std::shared_ptr<SBX::Classes::DataModel> get_data_model() const { return dataModel; }
	std::shared_ptr<SBX::Classes::Workspace> get_workspace() const;
	std::shared_ptr<SBX::Classes::Players> get_players() const;
	std::shared_ptr<SBX::Classes::RunService> get_run_service() const;

	// Execute a Luau script
	godot::String execute_script(const godot::String &code);

	// Execute a script with the 'script' global set
	godot::String run_script(const godot::String &code, SbxPart *script_parent);

	// Create local player
	void create_local_player(int64_t user_id, const godot::String &display_name);

	// Fire RunService signals (call from _process)
	void fire_heartbeat(double delta);
	void fire_stepped(double time, double delta);

	// GC control
	void gc_step(int step_size);
	int get_gc_memory() const;

	// Singleton access
	static SbxRuntime *get_singleton() { return singleton; }

protected:
	static void _bind_methods();

private:
	std::unique_ptr<SBX::LuauRuntime> runtime;
	std::shared_ptr<SBX::Classes::DataModel> dataModel;
	bool initialized = false;
	double elapsedTime = 0.0;

	static SbxRuntime *singleton;

	void initialize_runtime();
	void setup_data_model();
};

} // namespace SbxGD
