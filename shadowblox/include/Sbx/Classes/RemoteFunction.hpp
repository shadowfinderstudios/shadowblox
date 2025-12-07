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
 * @brief Callback type for handling network function calls from the engine.
 *
 * The callback receives:
 * - functionName: The name of the RemoteFunction
 * - targetId: For InvokeClient, the player ID to call on
 * - data: Serialized Luau values
 * Returns: Serialized return values
 */
using NetworkFunctionCallback = std::function<std::vector<uint8_t>(const char *functionName, int64_t targetId, const std::vector<uint8_t> &data)>;

/**
 * @brief This class implements Roblox's [`RemoteFunction`](https://create.roblox.com/docs/reference/engine/classes/RemoteFunction)
 * class.
 *
 * RemoteFunction is used for two-way communication between the server and clients.
 * Unlike RemoteEvent, it waits for a response from the other side.
 */
class RemoteFunction : public Instance {
	SBXCLASS(RemoteFunction, Instance, MemoryCategory::Instances);

public:
	RemoteFunction();
	~RemoteFunction() override = default;

	// Invoke methods (these yield until response is received)
	int InvokeServer(lua_State *L);
	int InvokeClient(std::shared_ptr<Player> player, lua_State *L);

	// Luau bindings
	static int InvokeServerLuau(lua_State *L);
	static int InvokeClientLuau(lua_State *L);

	// Callback setters (called from Luau)
	void SetOnServerInvoke(lua_State *L);
	void SetOnClientInvoke(lua_State *L);

	// Called by engine when an invoke request is received
	std::vector<uint8_t> HandleServerInvoke(std::shared_ptr<Player> player, lua_State *L, const std::vector<uint8_t> &data);
	std::vector<uint8_t> HandleClientInvoke(lua_State *L, const std::vector<uint8_t> &data);

	// Set network callback
	static void SetNetworkCallback(NetworkFunctionCallback callback);
	static NetworkFunctionCallback GetNetworkCallback();

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();

		// Server-only method
		ClassDB::BindLuauMethod<T, "InvokeClient", void(std::shared_ptr<Player>),
				&T::InvokeClientLuau, NoneSecurity, ThreadSafety::Unsafe>({ MemberTag::Yields }, "player");

		// Client-only method
		ClassDB::BindLuauMethod<T, "InvokeServer", void(),
				&T::InvokeServerLuau, NoneSecurity, ThreadSafety::Unsafe>({ MemberTag::Yields });

		// Callbacks (set via assignment)
		ClassDB::BindCallback<T, "OnServerInvoke", void(std::shared_ptr<Player>),
				&RemoteFunction::SetOnServerInvoke, NoneSecurity, ThreadSafety::Unsafe>({}, "player");

		ClassDB::BindCallback<T, "OnClientInvoke", void(),
				&RemoteFunction::SetOnClientInvoke, NoneSecurity, ThreadSafety::Unsafe>({});
	}

private:
	static NetworkFunctionCallback networkCallback;

	int onServerInvokeRef = LUA_NOREF;
	int onClientInvokeRef = LUA_NOREF;
	lua_State *serverInvokeState = nullptr;
	lua_State *clientInvokeState = nullptr;

	// Serialization helpers (reuse RemoteEvent's format)
	std::vector<uint8_t> SerializeArgs(lua_State *L, int argStart, int argCount);
	int DeserializeArgs(lua_State *L, const std::vector<uint8_t> &data);
};

} // namespace SBX::Classes
