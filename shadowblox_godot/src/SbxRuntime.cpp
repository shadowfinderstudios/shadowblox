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
#include "godot_cpp/variant/vector3.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/color.hpp"

#include "Luau/Compiler.h"
#include "lua.h"
#include "lualib.h"

#include "Sbx/Classes/DataModel.hpp"
#include "Sbx/Classes/Humanoid.hpp"
#include "Sbx/Classes/Model.hpp"
#include "Sbx/Classes/Part.hpp"
#include "Sbx/Classes/Players.hpp"
#include "Sbx/Classes/Player.hpp"
#include "Sbx/Classes/RemoteEvent.hpp"
#include "Sbx/Classes/RemoteFunction.hpp"
#include "Sbx/Classes/RunService.hpp"
#include "Sbx/Classes/Script.hpp"
#include "Sbx/Classes/Workspace.hpp"
#include "Sbx/DataTypes/Color3.hpp"
#include "Sbx/GodotBridge.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"
#include "Sbx/Runtime/Logger.hpp"
#include "Sbx/Runtime/SignalEmitter.hpp"
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
	godot::UtilityFunctions::print("[SbxRuntime] Destructor called");
	// Cleanup should have already happened in _exit_tree()
	// Just clear singleton as a safety measure
	if (singleton == this) {
		singleton = nullptr;
	}
	godot::UtilityFunctions::print("[SbxRuntime] Destructor complete");
}

void SbxRuntime::_ready() {
	initialize_runtime();
	setup_data_model();
}

void SbxRuntime::_exit_tree() {
	godot::UtilityFunctions::print("[SbxRuntime] _exit_tree() called - beginning cleanup");

	// Stop RunService first to prevent any more signals from firing
	if (dataModel) {
		godot::UtilityFunctions::print("[SbxRuntime] Stopping RunService...");
		auto runService = dataModel->GetRunService();
		if (runService) {
			runService->Stop();
			godot::UtilityFunctions::print("[SbxRuntime] RunService stopped");
		}
	}

	// Enable shutdown mode - this prevents SignalEmitter from accessing
	// lua_State pointers during Instance destruction
	godot::UtilityFunctions::print("[SbxRuntime] Enabling shutdown mode...");
	SBX::SignalEmitter::SetShutdownMode(true);

	// Destroy the Luau runtime first
	godot::UtilityFunctions::print("[SbxRuntime] Resetting runtime...");
	runtime.reset();
	godot::UtilityFunctions::print("[SbxRuntime] runtime reset complete");

	// Now safe to clear the DataModel - signal emission is disabled
	godot::UtilityFunctions::print("[SbxRuntime] Resetting dataModel...");
	dataModel.reset();
	godot::UtilityFunctions::print("[SbxRuntime] dataModel reset complete");

	godot::UtilityFunctions::print("[SbxRuntime] Resetting logger...");
	logger.reset();
	godot::UtilityFunctions::print("[SbxRuntime] logger reset complete");

	// Clear singleton
	if (singleton == this) {
		singleton = nullptr;
	}
	godot::UtilityFunctions::print("[SbxRuntime] _exit_tree() complete");
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

	// Create logger for print/warn to work in Luau scripts
	logger = std::make_unique<SBX::Logger>();

	// Set the logger in both VMs' thread data
	for (int i = 0; i < SBX::VMMax; i++) {
		lua_State *L = runtime->GetVM(SBX::VMType(i));
		SBX::SbxThreadData *udata = SBX::luaSBX_getthreaddata(L);
		if (udata && udata->global) {
			udata->global->logger = logger.get();
		}
	}

	initialized = true;
	godot::UtilityFunctions::print("[SbxRuntime] Luau runtime initialized");
}

// Luau callback: setPlayerColor(userId, color)
static int luaSbx_setPlayerColor(lua_State *L) {
	int64_t userId = static_cast<int64_t>(luaL_checknumber(L, 1));
	SBX::DataTypes::Color3 *color = SBX::LuauStackOp<SBX::DataTypes::Color3 *>::Get(L, 2);
	if (!color) {
		luaL_error(L, "setPlayerColor: expected Color3 as second argument");
		return 0;
	}

	SbxRuntime *runtime = SbxRuntime::get_singleton();
	if (runtime) {
		runtime->set_player_color(userId, godot::Color(color->R, color->G, color->B));
	}
	return 0;
}

// Luau callback: setStatusText(text)
static int luaSbx_setStatusText(lua_State *L) {
	const char *text = luaL_checkstring(L, 1);

	SbxRuntime *runtime = SbxRuntime::get_singleton();
	if (runtime) {
		runtime->set_status_text(godot::String(text));
	}
	return 0;
}

void SbxRuntime::setup_data_model() {
	if (!runtime) return;

	// Create DataModel (the 'game' object)
	dataModel = SBX::Bridge::CreateDataModel();

	// Get the VM and register globals
	lua_State *L = runtime->GetVM(SBX::UserVM);
	SBX::Bridge::RegisterGlobals(L, dataModel);

	// Register Luau-to-GDScript callback functions
	lua_pushcfunction(L, luaSbx_setPlayerColor, "setPlayerColor");
	lua_setglobal(L, "setPlayerColor");

	lua_pushcfunction(L, luaSbx_setStatusText, "setStatusText");
	lua_setglobal(L, "setStatusText");

	// Start RunService so Heartbeat/Stepped signals fire
	auto runService = get_run_service();
	if (runService) {
		runService->Run();
	}

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

void SbxRuntime::set_is_server(bool is_server) {
	isServer = is_server;
	auto runService = get_run_service();
	if (runService) {
		runService->SetIsServer(is_server);
		runService->SetIsClient(!is_server);
	}
	godot::UtilityFunctions::print("[SbxRuntime] Set as server: ", is_server);
}

void SbxRuntime::set_is_client(bool is_client) {
	isClient = is_client;
	auto runService = get_run_service();
	if (runService) {
		runService->SetIsClient(is_client);
		runService->SetIsServer(!is_client);
	}
	godot::UtilityFunctions::print("[SbxRuntime] Set as client: ", is_client);
}

void SbxRuntime::create_player(int64_t user_id, const godot::String &display_name) {
	auto players = get_players();
	if (players) {
		// Use AddPlayer which properly adds to playersByUserId and fires PlayerAdded signal
		players->AddPlayer(user_id, display_name.utf8().get_data());
		godot::UtilityFunctions::print("[SbxRuntime] Created player: ", display_name, " (ID: ", user_id, ")");
	}
}

void SbxRuntime::remove_player(int64_t user_id) {
	auto players = get_players();
	if (players) {
		auto player = players->GetPlayerByUserId(user_id);
		if (player) {
			players->RemovePlayer(player);
			godot::UtilityFunctions::print("[SbxRuntime] Removed player ID: ", user_id);
		}
	}
}

std::shared_ptr<SBX::Classes::Player> SbxRuntime::get_player(int64_t user_id) const {
	auto players = get_players();
	if (players) {
		return players->GetPlayerByUserId(user_id);
	}
	return nullptr;
}

void SbxRuntime::on_network_event(const godot::String &event_name, int64_t sender_id, const godot::PackedByteArray &data) {
	// Find the RemoteEvent in the DataModel and fire the appropriate signal
	if (!dataModel || !runtime) return;

	// Get the sender player (for server events)
	std::shared_ptr<SBX::Classes::Player> sender;
	if (isServer) {
		auto players = get_players();
		if (players) {
			sender = players->GetPlayerByUserId(sender_id);
		}
	}

	// Convert data to std::vector
	std::vector<uint8_t> dataVec(data.ptr(), data.ptr() + data.size());

	// Search for the RemoteEvent in ReplicatedStorage
	auto replicatedStorage = dataModel->GetService("ReplicatedStorage");
	if (replicatedStorage) {
		auto remoteEvents = replicatedStorage->FindFirstChild("RemoteEvents");
		if (remoteEvents) {
			auto event = remoteEvents->FindFirstChild(event_name.utf8().get_data());
			if (event && event->IsA("RemoteEvent")) {
				auto remoteEvent = std::dynamic_pointer_cast<SBX::Classes::RemoteEvent>(event);
				if (remoteEvent) {
					lua_State *L = runtime->GetVM(SBX::UserVM);
					if (isServer) {
						remoteEvent->OnServerEvent(sender, L, dataVec);
					} else {
						remoteEvent->OnClientEvent(L, dataVec);
					}
				}
			}
		}
	}
}

void SbxRuntime::on_network_function(const godot::String &function_name, int64_t sender_id, const godot::PackedByteArray &data, godot::PackedByteArray &response) {
	// Find the RemoteFunction and invoke the callback
	if (!dataModel || !runtime) return;

	std::shared_ptr<SBX::Classes::Player> sender;
	if (isServer) {
		auto players = get_players();
		if (players) {
			sender = players->GetPlayerByUserId(sender_id);
		}
	}

	std::vector<uint8_t> dataVec(data.ptr(), data.ptr() + data.size());

	auto replicatedStorage = dataModel->GetService("ReplicatedStorage");
	if (replicatedStorage) {
		auto remoteFunctions = replicatedStorage->FindFirstChild("RemoteFunctions");
		if (remoteFunctions) {
			auto func = remoteFunctions->FindFirstChild(function_name.utf8().get_data());
			if (func && func->IsA("RemoteFunction")) {
				auto remoteFunction = std::dynamic_pointer_cast<SBX::Classes::RemoteFunction>(func);
				if (remoteFunction) {
					lua_State *L = runtime->GetVM(SBX::UserVM);
					std::vector<uint8_t> result;
					if (isServer) {
						result = remoteFunction->HandleServerInvoke(sender, L, dataVec);
					} else {
						result = remoteFunction->HandleClientInvoke(L, dataVec);
					}
					response.resize(result.size());
					for (size_t i = 0; i < result.size(); ++i) {
						response[i] = result[i];
					}
				}
			}
		}
	}
}

void SbxRuntime::load_character(int64_t user_id) {
	auto players = get_players();
	if (!players) return;

	auto player = players->GetPlayerByUserId(user_id);
	if (!player) return;

	auto workspace = get_workspace();
	if (!workspace) return;

	// Create a character model
	auto character = std::make_shared<SBX::Classes::Model>();
	character->SetSelf(character);
	character->SetName(player->GetDisplayName());

	// Create the HumanoidRootPart
	auto rootPart = std::make_shared<SBX::Classes::Part>();
	rootPart->SetSelf(rootPart);
	rootPart->SetName("HumanoidRootPart");
	rootPart->SetSize(SBX::DataTypes::Vector3(2.0, 2.0, 1.0));
	rootPart->SetAnchored(false);
	rootPart->SetCanCollide(true);
	rootPart->SetParent(character);
	character->SetPrimaryPart(rootPart);

	// Create Torso
	auto torso = std::make_shared<SBX::Classes::Part>();
	torso->SetSelf(torso);
	torso->SetName("Torso");
	torso->SetSize(SBX::DataTypes::Vector3(2.0, 2.0, 1.0));
	torso->SetAnchored(false);
	torso->SetParent(character);

	// Create Head
	auto head = std::make_shared<SBX::Classes::Part>();
	head->SetSelf(head);
	head->SetName("Head");
	head->SetSize(SBX::DataTypes::Vector3(1.0, 1.0, 1.0));
	head->SetAnchored(false);
	head->SetParent(character);

	// Create Humanoid
	auto humanoid = std::make_shared<SBX::Classes::Humanoid>();
	humanoid->SetSelf(humanoid);
	humanoid->SetParent(character);

	// Set the character's parent to workspace
	character->SetParent(workspace);

	// Assign character to player
	player->SetCharacter(character);

	godot::UtilityFunctions::print("[SbxRuntime] Loaded character for player: ", godot::String(player->GetDisplayName()));
}

void SbxRuntime::set_input_direction(int64_t user_id, godot::Vector3 direction) {
	auto player = get_player(user_id);
	if (!player) return;

	auto character = player->GetCharacter();
	if (!character) return;

	// Find the Humanoid and set MoveDirection
	auto humanoidInstance = character->FindFirstChild("Humanoid");
	if (humanoidInstance && humanoidInstance->IsA("Humanoid")) {
		auto humanoid = std::dynamic_pointer_cast<SBX::Classes::Humanoid>(humanoidInstance);
		if (humanoid) {
			humanoid->SetMoveDirection(SBX::DataTypes::Vector3(direction.x, direction.y, direction.z));
		}
	}
}

godot::Vector3 SbxRuntime::get_player_position(int64_t user_id) const {
	auto player = get_player(user_id);
	if (!player) return godot::Vector3();

	auto character = player->GetCharacter();
	if (!character) return godot::Vector3();

	auto rootPart = character->GetPrimaryPart();
	if (!rootPart) return godot::Vector3();

	SBX::DataTypes::Vector3 pos = rootPart->GetPosition();
	return godot::Vector3(pos.X, pos.Y, pos.Z);
}

void SbxRuntime::set_player_position(int64_t user_id, godot::Vector3 position) {
	auto player = get_player(user_id);
	if (!player) return;

	auto character = player->GetCharacter();
	if (!character) return;

	auto rootPart = character->GetPrimaryPart();
	if (!rootPart) return;

	rootPart->SetPosition(SBX::DataTypes::Vector3(position.x, position.y, position.z));
}

godot::Dictionary SbxRuntime::get_all_player_positions() const {
	godot::Dictionary result;
	auto players = get_players();
	if (!players) return result;

	// Iterate through all children of Players
	for (const auto &child : players->GetChildren()) {
		if (child->IsA("Player")) {
			auto player = std::dynamic_pointer_cast<SBX::Classes::Player>(child);
			if (player) {
				int64_t userId = player->GetUserId();
				auto character = player->GetCharacter();
				if (character) {
					auto rootPart = character->GetPrimaryPart();
					if (rootPart) {
						SBX::DataTypes::Vector3 pos = rootPart->GetPosition();
						result[userId] = godot::Vector3(pos.X, pos.Y, pos.Z);
					}
				}
			}
		}
	}
	return result;
}

void SbxRuntime::set_player_color(int64_t user_id, godot::Color color) {
	emit_signal("player_color_changed", user_id, color);
}

void SbxRuntime::set_status_text(const godot::String &text) {
	emit_signal("status_text_changed", text);
}

void SbxRuntime::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("execute_script", "code"), &SbxRuntime::execute_script);
	godot::ClassDB::bind_method(godot::D_METHOD("gc_step", "step_size"), &SbxRuntime::gc_step);
	godot::ClassDB::bind_method(godot::D_METHOD("get_gc_memory"), &SbxRuntime::get_gc_memory);
	godot::ClassDB::bind_method(godot::D_METHOD("create_local_player", "user_id", "display_name"), &SbxRuntime::create_local_player);
	godot::ClassDB::bind_method(godot::D_METHOD("fire_heartbeat", "delta"), &SbxRuntime::fire_heartbeat);
	godot::ClassDB::bind_method(godot::D_METHOD("fire_stepped", "time", "delta"), &SbxRuntime::fire_stepped);

	// Multiplayer methods
	godot::ClassDB::bind_method(godot::D_METHOD("set_is_server", "is_server"), &SbxRuntime::set_is_server);
	godot::ClassDB::bind_method(godot::D_METHOD("get_is_server"), &SbxRuntime::get_is_server);
	godot::ClassDB::bind_method(godot::D_METHOD("set_is_client", "is_client"), &SbxRuntime::set_is_client);
	godot::ClassDB::bind_method(godot::D_METHOD("get_is_client"), &SbxRuntime::get_is_client);
	godot::ClassDB::bind_method(godot::D_METHOD("create_player", "user_id", "display_name"), &SbxRuntime::create_player);
	godot::ClassDB::bind_method(godot::D_METHOD("remove_player", "user_id"), &SbxRuntime::remove_player);
	godot::ClassDB::bind_method(godot::D_METHOD("load_character", "user_id"), &SbxRuntime::load_character);
	godot::ClassDB::bind_method(godot::D_METHOD("on_network_event", "event_name", "sender_id", "data"), &SbxRuntime::on_network_event);

	// Input/Position bridging methods
	godot::ClassDB::bind_method(godot::D_METHOD("set_input_direction", "user_id", "direction"), &SbxRuntime::set_input_direction);
	godot::ClassDB::bind_method(godot::D_METHOD("get_player_position", "user_id"), &SbxRuntime::get_player_position);
	godot::ClassDB::bind_method(godot::D_METHOD("set_player_position", "user_id", "position"), &SbxRuntime::set_player_position);
	godot::ClassDB::bind_method(godot::D_METHOD("get_all_player_positions"), &SbxRuntime::get_all_player_positions);

	// Generic rendering control methods - Luau controls what GDScript renders
	godot::ClassDB::bind_method(godot::D_METHOD("set_player_color", "user_id", "color"), &SbxRuntime::set_player_color);
	godot::ClassDB::bind_method(godot::D_METHOD("set_status_text", "text"), &SbxRuntime::set_status_text);

	// Signals for network communication
	ADD_SIGNAL(godot::MethodInfo("network_event_to_server",
			godot::PropertyInfo(godot::Variant::STRING, "event_name"),
			godot::PropertyInfo(godot::Variant::PACKED_BYTE_ARRAY, "data")));
	ADD_SIGNAL(godot::MethodInfo("network_event_to_client",
			godot::PropertyInfo(godot::Variant::STRING, "event_name"),
			godot::PropertyInfo(godot::Variant::INT, "target_id"),
			godot::PropertyInfo(godot::Variant::PACKED_BYTE_ARRAY, "data")));
	ADD_SIGNAL(godot::MethodInfo("network_event_to_all_clients",
			godot::PropertyInfo(godot::Variant::STRING, "event_name"),
			godot::PropertyInfo(godot::Variant::PACKED_BYTE_ARRAY, "data")));
	ADD_SIGNAL(godot::MethodInfo("player_added",
			godot::PropertyInfo(godot::Variant::INT, "user_id"),
			godot::PropertyInfo(godot::Variant::STRING, "display_name")));
	ADD_SIGNAL(godot::MethodInfo("player_removing",
			godot::PropertyInfo(godot::Variant::INT, "user_id")));

	// Generic rendering control signals - Luau tells GDScript what to render
	ADD_SIGNAL(godot::MethodInfo("player_color_changed",
			godot::PropertyInfo(godot::Variant::INT, "user_id"),
			godot::PropertyInfo(godot::Variant::COLOR, "color")));
	ADD_SIGNAL(godot::MethodInfo("status_text_changed",
			godot::PropertyInfo(godot::Variant::STRING, "text")));
}

} // namespace SbxGD
