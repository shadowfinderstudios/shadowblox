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

#include "lua.h"

#include "Sbx/Classes/Instance.hpp"
#include "Sbx/DataTypes/Vector3.hpp"

namespace SBX::Classes {

class Part;

/**
 * @brief This class implements Roblox's [`Humanoid`](https://create.roblox.com/docs/reference/engine/classes/Humanoid)
 * class.
 *
 * Humanoid is the class that provides character functionality such as walking,
 * jumping, and health management. It is typically placed inside a character Model.
 */
class Humanoid : public Instance {
	SBXCLASS(Humanoid, Instance, MemoryCategory::Instances);

public:
	Humanoid();
	~Humanoid() override = default;

	// Health properties
	double GetHealth() const { return health; }
	void SetHealth(double value);

	double GetMaxHealth() const { return maxHealth; }
	void SetMaxHealth(double value);

	// Movement properties
	double GetWalkSpeed() const { return walkSpeed; }
	void SetWalkSpeed(double value);

	double GetJumpPower() const { return jumpPower; }
	void SetJumpPower(double value);

	double GetJumpHeight() const { return jumpHeight; }
	void SetJumpHeight(double value);

	bool GetJump() const { return jump; }
	void SetJump(bool value);

	// State
	bool GetSit() const { return sit; }
	void SetSit(bool value);

	bool GetPlatformStand() const { return platformStand; }
	void SetPlatformStand(bool value);

	bool GetAutoRotate() const { return autoRotate; }
	void SetAutoRotate(bool value);

	// Movement direction (for replication)
	DataTypes::Vector3 GetMoveDirection() const { return moveDirection; }
	void SetMoveDirection(DataTypes::Vector3 value);

	// Methods
	void TakeDamage(double amount);
	static int TakeDamageLuau(lua_State *L);

	void MoveTo(DataTypes::Vector3 location);
	void MoveToWithPart(DataTypes::Vector3 location, std::shared_ptr<Part> part);
	static int MoveToLuau(lua_State *L);

	// Luau property accessors for Vector3 properties
	static int GetMoveDirectionLuau(lua_State *L);

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();

		// Vector3 properties via custom index
		LuauClassBinder<T>::AddIndexOverride(HumanoidIndexOverride);

		// Register MoveDirection for reflection
		ClassDB::BindPropertyNotScriptable<T, "MoveDirection", "Humanoid",
				&Humanoid::GetMoveDirection, &Humanoid::SetMoveDirection,
				ThreadSafety::Unsafe, true, true>({});

		// Health properties
		ClassDB::BindProperty<T, "Health", "Humanoid", &Humanoid::GetHealth, NoneSecurity,
				&Humanoid::SetHealth, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "MaxHealth", "Humanoid", &Humanoid::GetMaxHealth, NoneSecurity,
				&Humanoid::SetMaxHealth, NoneSecurity, ThreadSafety::Unsafe, true, true>({});

		// Movement properties
		ClassDB::BindProperty<T, "WalkSpeed", "Humanoid", &Humanoid::GetWalkSpeed, NoneSecurity,
				&Humanoid::SetWalkSpeed, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "JumpPower", "Humanoid", &Humanoid::GetJumpPower, NoneSecurity,
				&Humanoid::SetJumpPower, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "JumpHeight", "Humanoid", &Humanoid::GetJumpHeight, NoneSecurity,
				&Humanoid::SetJumpHeight, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "Jump", "Humanoid", &Humanoid::GetJump, NoneSecurity,
				&Humanoid::SetJump, NoneSecurity, ThreadSafety::Unsafe, true, true>({});

		// State properties
		ClassDB::BindProperty<T, "Sit", "Humanoid", &Humanoid::GetSit, NoneSecurity,
				&Humanoid::SetSit, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "PlatformStand", "Humanoid", &Humanoid::GetPlatformStand, NoneSecurity,
				&Humanoid::SetPlatformStand, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "AutoRotate", "Humanoid", &Humanoid::GetAutoRotate, NoneSecurity,
				&Humanoid::SetAutoRotate, NoneSecurity, ThreadSafety::Unsafe, true, true>({});

		// Methods
		ClassDB::BindLuauMethod<T, "TakeDamage", void(double),
				&T::TakeDamageLuau, NoneSecurity, ThreadSafety::Unsafe>({}, "amount");

		ClassDB::BindLuauMethod<T, "MoveTo", void(DataTypes::Vector3),
				&T::MoveToLuau, NoneSecurity, ThreadSafety::Unsafe>({}, "location");

		// Signals
		ClassDB::BindSignal<T, "Died", void(), NoneSecurity>({});
		ClassDB::BindSignal<T, "HealthChanged", void(double), NoneSecurity>({}, "health");
		ClassDB::BindSignal<T, "Jumping", void(bool), NoneSecurity>({}, "active");
		ClassDB::BindSignal<T, "Running", void(double), NoneSecurity>({}, "speed");
		ClassDB::BindSignal<T, "MoveToFinished", void(bool), NoneSecurity>({}, "reached");
		ClassDB::BindSignal<T, "Touched", void(std::shared_ptr<Part>, std::shared_ptr<Part>), NoneSecurity>({}, "touchingPart", "humanoidPart");
	}

private:
	double health = 100.0;
	double maxHealth = 100.0;
	double walkSpeed = 16.0;
	double jumpPower = 50.0;
	double jumpHeight = 7.2;
	bool jump = false;
	bool sit = false;
	bool platformStand = false;
	bool autoRotate = true;
	DataTypes::Vector3 moveDirection = DataTypes::Vector3::zero;

	// Custom index override for Vector3 properties
	static int HumanoidIndexOverride(lua_State *L, const char *propName);
};

} // namespace SBX::Classes
