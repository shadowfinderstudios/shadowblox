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

#include "Sbx/Classes/Player.hpp"

#include <cstring>

#include "lua.h"

#include "Sbx/Classes/Model.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::Classes {

Player::Player() :
		Instance() {
	SetName("Player");
}

void Player::SetCharacter(std::shared_ptr<Model> model) {
	auto oldCharacter = character.lock();
	if (oldCharacter == model) {
		return;
	}

	if (oldCharacter) {
		Emit<Player>("CharacterRemoving", oldCharacter);
	}

	character = model;
	Changed<Player>("Character");

	if (model) {
		Emit<Player>("CharacterAdded", model);
	}
}

void Player::SetUserId(int64_t id) {
	userId = id;
	Changed<Player>("UserId");
}

void Player::SetDisplayName(const char *name) {
	displayName = name ? name : "";
	Changed<Player>("DisplayName");
}

void Player::SetTeamColor(const char *color) {
	teamColor = color ? color : "";
	Changed<Player>("TeamColor");
}

int Player::GetCharacterLuau(lua_State *L) {
	Player *self = LuauStackOp<Player *>::Check(L, 1);
	std::shared_ptr<Model> character = self->GetCharacter();
	if (character) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, character);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Player::SetCharacterLuau(lua_State *L) {
	Player *self = LuauStackOp<Player *>::Check(L, 1);
	if (lua_isnil(L, 3)) {
		self->SetCharacter(nullptr);
	} else {
		std::shared_ptr<Model> model = LuauStackOp<std::shared_ptr<Model>>::Check(L, 3);
		self->SetCharacter(model);
	}
	return 0;
}

int Player::PlayerIndexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "Character") == 0) {
		return GetCharacterLuau(L);
	}
	return 0; // Not handled
}

bool Player::PlayerNewindexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "Character") == 0) {
		SetCharacterLuau(L);
		return true;
	}
	return false; // Not handled
}

} // namespace SBX::Classes
