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

#include "Sbx/Classes/Players.hpp"

#include <cstring>

#include "lua.h"

#include "Sbx/Classes/Model.hpp"
#include "Sbx/Classes/Player.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::Classes {

Players::Players() :
		Instance() {
	SetName("Players");
}

void Players::SetLocalPlayer(std::shared_ptr<Player> player) {
	localPlayer = player;
}

std::vector<std::shared_ptr<Player>> Players::GetPlayers() const {
	std::vector<std::shared_ptr<Player>> result;

	for (const auto &child : GetChildren()) {
		auto player = std::dynamic_pointer_cast<Player>(child);
		if (player) {
			result.push_back(player);
		}
	}

	return result;
}

std::shared_ptr<Player> Players::GetPlayerByUserId(int64_t userId) const {
	auto it = playersByUserId.find(userId);
	if (it != playersByUserId.end()) {
		return it->second.lock();
	}
	return nullptr;
}

std::shared_ptr<Player> Players::GetPlayerFromCharacter(std::shared_ptr<Instance> character) const {
	if (!character) {
		return nullptr;
	}

	// The character could be a Model or any Instance that is a player's character
	// Check each player to see if their character matches
	for (const auto &child : GetChildren()) {
		auto player = std::dynamic_pointer_cast<Player>(child);
		if (player && player->GetCharacter() == character) {
			return player;
		}
	}

	// Also check if the character's parent matches (for Parts inside character)
	auto parent = character->GetParent();
	if (parent) {
		for (const auto &child : GetChildren()) {
			auto player = std::dynamic_pointer_cast<Player>(child);
			if (player) {
				auto playerChar = player->GetCharacter();
				if (playerChar && playerChar == parent) {
					return player;
				}
			}
		}
	}

	return nullptr;
}

std::shared_ptr<Player> Players::CreateLocalPlayer(int64_t userId, const char *displayName) {
	auto player = std::make_shared<Player>();
	player->SetSelf(player);
	player->SetUserId(userId);
	player->SetDisplayName(displayName);
	player->SetParent(GetSelf());

	playersByUserId[userId] = player;
	localPlayer = player;

	Emit<Players>("PlayerAdded", player);

	return player;
}

std::shared_ptr<Player> Players::AddPlayer(int64_t userId, const char *displayName) {
	// Check if player already exists
	auto it = playersByUserId.find(userId);
	if (it != playersByUserId.end()) {
		auto existing = it->second.lock();
		if (existing) {
			return existing;
		}
	}

	auto player = std::make_shared<Player>();
	player->SetSelf(player);
	player->SetUserId(userId);
	player->SetDisplayName(displayName);
	player->SetParent(GetSelf());

	playersByUserId[userId] = player;

	Emit<Players>("PlayerAdded", player);

	return player;
}

void Players::RemovePlayer(std::shared_ptr<Player> player) {
	if (!player) {
		return;
	}

	Emit<Players>("PlayerRemoving", player);

	playersByUserId.erase(player->GetUserId());

	if (localPlayer.lock() == player) {
		localPlayer.reset();
	}

	player->SetParent(nullptr);
}

int Players::GetPlayersLuau(lua_State *L) {
	Players *self = LuauStackOp<Players *>::Check(L, 1);
	std::vector<std::shared_ptr<Player>> players = self->GetPlayers();

	lua_createtable(L, static_cast<int>(players.size()), 0);
	for (size_t i = 0; i < players.size(); ++i) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, players[i]);
		lua_rawseti(L, -2, static_cast<int>(i + 1));
	}

	return 1;
}

int Players::GetPlayerByUserIdLuau(lua_State *L) {
	Players *self = LuauStackOp<Players *>::Check(L, 1);
	int64_t userId = static_cast<int64_t>(luaL_checknumber(L, 2));

	std::shared_ptr<Player> player = self->GetPlayerByUserId(userId);
	if (player) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, player);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

int Players::GetPlayerFromCharacterLuau(lua_State *L) {
	Players *self = LuauStackOp<Players *>::Check(L, 1);

	std::shared_ptr<Instance> character;
	if (!lua_isnil(L, 2)) {
		character = LuauStackOp<std::shared_ptr<Instance>>::Check(L, 2);
	}

	std::shared_ptr<Player> player = self->GetPlayerFromCharacter(character);
	if (player) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, player);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

int Players::GetLocalPlayerLuau(lua_State *L) {
	Players *self = LuauStackOp<Players *>::Check(L, 1);
	std::shared_ptr<Player> player = self->GetLocalPlayer();
	if (player) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, player);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Players::PlayersIndexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "LocalPlayer") == 0) {
		return GetLocalPlayerLuau(L);
	}
	return 0; // Not handled
}

} // namespace SBX::Classes
