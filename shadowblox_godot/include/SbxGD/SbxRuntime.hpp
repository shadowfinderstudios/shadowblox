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
class Logger;
}

namespace SBX::Classes {
class DataModel;
class Workspace;
class Players;
class Player;
class RunService;
class Script;
class Instance;
class RemoteEvent;
class RemoteFunction;
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
	void _exit_tree() override;

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

	// Multiplayer support
	void set_is_server(bool is_server);
	bool get_is_server() const { return isServer; }
	void set_is_client(bool is_client);
	bool get_is_client() const { return isClient; }

	// Create a player (server-side)
	void create_player(int64_t user_id, const godot::String &display_name);

	// Remove a player (server-side)
	void remove_player(int64_t user_id);

	// Get player by user ID
	std::shared_ptr<SBX::Classes::Player> get_player(int64_t user_id) const;

	// Network event handling
	void on_network_event(const godot::String &event_name, int64_t sender_id, const godot::PackedByteArray &data);
	void on_network_function(const godot::String &function_name, int64_t sender_id, const godot::PackedByteArray &data, godot::PackedByteArray &response);

	// Load character for a player
	void load_character(int64_t user_id);

	// Input/Position bridging for GDScript integration
	void set_input_direction(int64_t user_id, godot::Vector3 direction);
	godot::Vector3 get_player_position(int64_t user_id) const;
	void set_player_position(int64_t user_id, godot::Vector3 position);
	godot::Dictionary get_all_player_positions() const;

	// Generic rendering control - Luau tells GDScript what to render
	void set_player_color(int64_t user_id, godot::Color color);
	void set_status_text(const godot::String &text);

protected:
	static void _bind_methods();

private:
	std::unique_ptr<SBX::LuauRuntime> runtime;
	std::unique_ptr<SBX::Logger> logger;
	std::shared_ptr<SBX::Classes::DataModel> dataModel;
	bool initialized = false;
	double elapsedTime = 0.0;
	bool isServer = true;
	bool isClient = false;

	static SbxRuntime *singleton;

	void initialize_runtime();
	void setup_data_model();
};

} // namespace SbxGD
