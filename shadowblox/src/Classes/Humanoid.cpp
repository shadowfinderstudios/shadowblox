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

#include "Sbx/Classes/Humanoid.hpp"

#include <cstring>

#include "lua.h"

#include "Sbx/Classes/Part.hpp"
#include "Sbx/DataTypes/Vector3.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::Classes {

Humanoid::Humanoid() :
		Instance() {
	SetName("Humanoid");
}

void Humanoid::SetHealth(double value) {
	double oldHealth = health;
	health = value;

	if (health < 0) {
		health = 0;
	}
	if (health > maxHealth) {
		health = maxHealth;
	}

	if (health != oldHealth) {
		Emit<Humanoid>("HealthChanged", health);
		Changed<Humanoid>("Health");

		if (health <= 0 && oldHealth > 0) {
			Emit<Humanoid>("Died");
		}
	}
}

void Humanoid::SetMaxHealth(double value) {
	maxHealth = value;
	if (maxHealth < 0) {
		maxHealth = 0;
	}
	if (health > maxHealth) {
		SetHealth(maxHealth);
	}
	Changed<Humanoid>("MaxHealth");
}

void Humanoid::SetWalkSpeed(double value) {
	walkSpeed = value;
	if (walkSpeed < 0) {
		walkSpeed = 0;
	}
	Changed<Humanoid>("WalkSpeed");
}

void Humanoid::SetJumpPower(double value) {
	jumpPower = value;
	Changed<Humanoid>("JumpPower");
}

void Humanoid::SetJumpHeight(double value) {
	jumpHeight = value;
	Changed<Humanoid>("JumpHeight");
}

void Humanoid::SetJump(bool value) {
	bool wasJumping = jump;
	jump = value;
	if (jump && !wasJumping) {
		Emit<Humanoid>("Jumping", true);
	}
	Changed<Humanoid>("Jump");
}

void Humanoid::SetSit(bool value) {
	sit = value;
	Changed<Humanoid>("Sit");
}

void Humanoid::SetPlatformStand(bool value) {
	platformStand = value;
	Changed<Humanoid>("PlatformStand");
}

void Humanoid::SetAutoRotate(bool value) {
	autoRotate = value;
	Changed<Humanoid>("AutoRotate");
}

void Humanoid::SetMoveDirection(DataTypes::Vector3 value) {
	moveDirection = value;
	Changed<Humanoid>("MoveDirection");
}

void Humanoid::TakeDamage(double amount) {
	if (amount > 0) {
		SetHealth(health - amount);
	}
}

void Humanoid::MoveTo(DataTypes::Vector3 location) {
	// This would be implemented by the game engine
	// For now, just emit that we want to move
	moveDirection = location;
}

void Humanoid::MoveToWithPart(DataTypes::Vector3 location, std::shared_ptr<Part> part) {
	// Enhanced MoveTo with a target part
	MoveTo(location);
}

int Humanoid::TakeDamageLuau(lua_State *L) {
	Humanoid *self = LuauStackOp<Humanoid *>::Check(L, 1);
	double amount = luaL_checknumber(L, 2);
	self->TakeDamage(amount);
	return 0;
}

int Humanoid::MoveToLuau(lua_State *L) {
	Humanoid *self = LuauStackOp<Humanoid *>::Check(L, 1);

	if (LuauStackOp<DataTypes::Vector3>::Is(L, 2)) {
		DataTypes::Vector3 location = LuauStackOp<DataTypes::Vector3>::Check(L, 2);
		if (lua_gettop(L) >= 3 && LuauStackOp<std::shared_ptr<Part>>::Is(L, 3)) {
			std::shared_ptr<Part> part = LuauStackOp<std::shared_ptr<Part>>::Check(L, 3);
			self->MoveToWithPart(location, part);
		} else {
			self->MoveTo(location);
		}
	}

	return 0;
}

int Humanoid::GetMoveDirectionLuau(lua_State *L) {
	Humanoid *self = LuauStackOp<Humanoid *>::Check(L, 1);
	LuauStackOp<DataTypes::Vector3>::Push(L, self->GetMoveDirection());
	return 1;
}

int Humanoid::HumanoidIndexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "MoveDirection") == 0) {
		return GetMoveDirectionLuau(L);
	}
	return 0;
}

} // namespace SBX::Classes
