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
#include <unordered_map>
#include <vector>

#include "lua.h"

#include "Sbx/Classes/Instance.hpp"
#include "Sbx/Classes/Player.hpp"

namespace SBX::Classes {

/**
 * @brief This class implements Roblox's [`Players`](https://create.roblox.com/docs/reference/engine/classes/Players)
 * service.
 *
 * Players is a service that contains all Player objects for players currently
 * in the game. It provides methods for getting players and handling player events.
 */
class Players : public Instance {
	SBXCLASS(Players, Instance, MemoryCategory::Instances, ClassTag::NotCreatable, ClassTag::Service);

public:
	Players();
	~Players() override = default;

	// Get local player (client only, returns nil on server)
	std::shared_ptr<Player> GetLocalPlayer() const { return localPlayer.lock(); }
	void SetLocalPlayer(std::shared_ptr<Player> player);

	// Get all players
	std::vector<std::shared_ptr<Player>> GetPlayers() const;
	static int GetPlayersLuau(lua_State *L);

	// Get player by UserId
	std::shared_ptr<Player> GetPlayerByUserId(int64_t userId) const;
	static int GetPlayerByUserIdLuau(lua_State *L);

	// Get player from character model
	std::shared_ptr<Player> GetPlayerFromCharacter(std::shared_ptr<Instance> character) const;
	static int GetPlayerFromCharacterLuau(lua_State *L);

	// Create and add a player (typically called by game engine)
	std::shared_ptr<Player> CreateLocalPlayer(int64_t userId, const char *displayName);

	// Add a remote player (server creating player for a connected client)
	std::shared_ptr<Player> AddPlayer(int64_t userId, const char *displayName);

	// Remove a player
	void RemovePlayer(std::shared_ptr<Player> player);

	// Max players property
	int GetMaxPlayers() const { return maxPlayers; }
	void SetMaxPlayers(int value) { maxPlayers = value; }

	// Luau property accessor for LocalPlayer
	static int GetLocalPlayerLuau(lua_State *L);

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();

		// LocalPlayer via custom index
		LuauClassBinder<T>::AddIndexOverride(PlayersIndexOverride);

		// Register LocalPlayer in ClassDB for reflection (not scriptable via normal path)
		ClassDB::BindPropertyNotScriptable<T, "LocalPlayer", "Players",
				&Players::GetLocalPlayer, nullptr,
				ThreadSafety::Safe, true, false>({});

		// Simple properties
		ClassDB::BindProperty<T, "MaxPlayers", "Players", &Players::GetMaxPlayers, NoneSecurity,
				&Players::SetMaxPlayers, NoneSecurity, ThreadSafety::Safe, true, true>({});

		// Methods
		ClassDB::BindLuauMethod<T, "GetPlayers", std::vector<std::shared_ptr<Player>>(),
				&T::GetPlayersLuau, NoneSecurity, ThreadSafety::Safe>({});

		ClassDB::BindLuauMethod<T, "GetPlayerByUserId", std::shared_ptr<Player>(int64_t),
				&T::GetPlayerByUserIdLuau, NoneSecurity, ThreadSafety::Safe>({}, "userId");

		ClassDB::BindLuauMethod<T, "GetPlayerFromCharacter", std::shared_ptr<Player>(std::shared_ptr<Instance>),
				&T::GetPlayerFromCharacterLuau, NoneSecurity, ThreadSafety::Safe>({}, "character");

		// Signals
		ClassDB::BindSignal<T, "PlayerAdded", void(std::shared_ptr<Player>), NoneSecurity>({}, "player");
		ClassDB::BindSignal<T, "PlayerRemoving", void(std::shared_ptr<Player>), NoneSecurity>({}, "player");
	}

private:
	std::weak_ptr<Player> localPlayer;
	std::unordered_map<int64_t, std::weak_ptr<Player>> playersByUserId;
	int maxPlayers = 50;

	// Custom index override for LocalPlayer property
	static int PlayersIndexOverride(lua_State *L, const char *propName);
};

} // namespace SBX::Classes
