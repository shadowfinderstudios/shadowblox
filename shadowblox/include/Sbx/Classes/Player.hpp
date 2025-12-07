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
#include <string>

#include "lua.h"

#include "Sbx/Classes/Instance.hpp"
#include "Sbx/Classes/Model.hpp"

namespace SBX::Classes {

/**
 * @brief This class implements Roblox's [`Player`](https://create.roblox.com/docs/reference/engine/classes/Player)
 * class.
 *
 * Player represents a player in the game. It contains information about the player
 * and provides access to their character.
 */
class Player : public Instance {
	SBXCLASS(Player, Instance, MemoryCategory::Instances, ClassTag::NotCreatable);

public:
	Player();
	~Player() override = default;

	// Character property - the player's character model
	std::shared_ptr<Model> GetCharacter() const { return character.lock(); }
	void SetCharacter(std::shared_ptr<Model> model);

	// UserId property
	int64_t GetUserId() const { return userId; }
	void SetUserId(int64_t id);

	// DisplayName property
	const char *GetDisplayName() const { return displayName.c_str(); }
	void SetDisplayName(const char *name);

	// Team property (simplified - just a BrickColor string for now)
	const char *GetTeamColor() const { return teamColor.c_str(); }
	void SetTeamColor(const char *color);

	// Luau property accessors for Character (needs special handling)
	static int GetCharacterLuau(lua_State *L);
	static int SetCharacterLuau(lua_State *L);

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();

		// Character via custom index/newindex
		LuauClassBinder<T>::AddIndexOverride(PlayerIndexOverride);
		LuauClassBinder<T>::AddNewindexOverride(PlayerNewindexOverride);

		// Register Character in ClassDB for reflection (not scriptable via normal path)
		ClassDB::BindPropertyNotScriptable<T, "Character", "Player",
				&Player::GetCharacter, &Player::SetCharacter,
				ThreadSafety::Unsafe, true, true>({});

		// Simple properties
		ClassDB::BindProperty<T, "UserId", "Player", &Player::GetUserId, NoneSecurity,
				&Player::SetUserId, NoneSecurity, ThreadSafety::Safe, true, true>({});
		ClassDB::BindProperty<T, "DisplayName", "Player", &Player::GetDisplayName, NoneSecurity,
				&Player::SetDisplayName, NoneSecurity, ThreadSafety::Safe, true, true>({});
		ClassDB::BindProperty<T, "TeamColor", "Player", &Player::GetTeamColor, NoneSecurity,
				&Player::SetTeamColor, NoneSecurity, ThreadSafety::Safe, true, true>({});

		// Signals
		ClassDB::BindSignal<T, "CharacterAdded", void(std::shared_ptr<Model>), NoneSecurity>({}, "character");
		ClassDB::BindSignal<T, "CharacterRemoving", void(std::shared_ptr<Model>), NoneSecurity>({}, "character");
	}

private:
	std::weak_ptr<Model> character;
	int64_t userId = 0;
	std::string displayName;
	std::string teamColor;

	// Custom index/newindex overrides for Character property
	static int PlayerIndexOverride(lua_State *L, const char *propName);
	static bool PlayerNewindexOverride(lua_State *L, const char *propName);
};

} // namespace SBX::Classes
