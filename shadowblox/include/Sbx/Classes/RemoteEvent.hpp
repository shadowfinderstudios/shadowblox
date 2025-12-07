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

#include <functional>
#include <memory>
#include <vector>

#include "lua.h"

#include "Sbx/Classes/Instance.hpp"

namespace SBX::Classes {

class Player;

/**
 * @brief Callback type for handling network events from the engine.
 *
 * The callback receives:
 * - eventName: The name of the RemoteEvent
 * - targetId: For FireClient, the player ID to send to (-1 for all clients)
 * - data: Serialized Luau values
 */
using NetworkEventCallback = std::function<void(const char *eventName, int64_t targetId, const std::vector<uint8_t> &data)>;

/**
 * @brief This class implements Roblox's [`RemoteEvent`](https://create.roblox.com/docs/reference/engine/classes/RemoteEvent)
 * class.
 *
 * RemoteEvent is used for one-way communication between the server and clients.
 * On the server, use FireClient or FireAllClients to send events to clients.
 * On the client, use FireServer to send events to the server.
 *
 * Note: The actual network transport is handled by the game engine (Godot).
 * This class provides the Luau API and serialization layer.
 */
class RemoteEvent : public Instance {
	SBXCLASS(RemoteEvent, Instance, MemoryCategory::Instances);

public:
	RemoteEvent();
	~RemoteEvent() override = default;

	// Server methods
	void FireClient(std::shared_ptr<Player> player, lua_State *L, int argStart, int argCount);
	void FireAllClients(lua_State *L, int argStart, int argCount);

	// Client methods
	void FireServer(lua_State *L, int argStart, int argCount);

	// Luau bindings
	static int FireClientLuau(lua_State *L);
	static int FireAllClientsLuau(lua_State *L);
	static int FireServerLuau(lua_State *L);

	// Called by the engine when a network event is received
	void OnServerEvent(std::shared_ptr<Player> player, lua_State *L, const std::vector<uint8_t> &data);
	void OnClientEvent(lua_State *L, const std::vector<uint8_t> &data);

	// Set network callback (called by engine to register transport)
	static void SetNetworkCallback(NetworkEventCallback callback);
	static NetworkEventCallback GetNetworkCallback();

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();

		// Server-only methods (will error if called on client)
		ClassDB::BindLuauMethod<T, "FireClient", void(std::shared_ptr<Player>),
				&T::FireClientLuau, NoneSecurity, ThreadSafety::Unsafe>({}, "player");

		ClassDB::BindLuauMethod<T, "FireAllClients", void(),
				&T::FireAllClientsLuau, NoneSecurity, ThreadSafety::Unsafe>({});

		// Client-only method (will error if called on server)
		ClassDB::BindLuauMethod<T, "FireServer", void(),
				&T::FireServerLuau, NoneSecurity, ThreadSafety::Unsafe>({});

		// Signals
		ClassDB::BindSignal<T, "OnServerEvent", void(std::shared_ptr<Player>), NoneSecurity>({}, "player");
		ClassDB::BindSignal<T, "OnClientEvent", void(), NoneSecurity>({});
	}

private:
	static NetworkEventCallback networkCallback;

	// Serialize Luau values from stack to binary
	std::vector<uint8_t> SerializeArgs(lua_State *L, int argStart, int argCount);

	// Deserialize binary to Luau values and push to stack
	int DeserializeArgs(lua_State *L, const std::vector<uint8_t> &data);
};

} // namespace SBX::Classes
