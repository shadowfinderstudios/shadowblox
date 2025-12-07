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
#include <vector>

#include "lua.h"

#include "Sbx/Classes/Object.hpp"

namespace SBX::Classes {

/**
 * @brief This class implements Roblox's [`Instance`](https://create.roblox.com/docs/reference/engine/classes/Instance)
 * class.
 *
 * Instance is the base class for all classes in the Roblox class hierarchy that can be part of the
 * DataModel tree. It provides methods for managing parent-child relationships, finding descendants,
 * and other common functionality.
 */
class Instance : public Object {
	SBXCLASS(Instance, Object, MemoryCategory::Instances, ClassTag::NotCreatable, ClassTag::NotReplicated);

public:
	Instance();
	~Instance() override;

	// Properties
	const char *GetName() const;
	void SetName(const char *newName);

	std::shared_ptr<Instance> GetParent() const;
	void SetParent(std::shared_ptr<Instance> newParent);

	// Methods
	std::vector<std::shared_ptr<Instance>> GetChildren() const;
	std::vector<std::shared_ptr<Instance>> GetDescendants() const;
	std::shared_ptr<Instance> FindFirstChild(const char *name, bool recursive = false) const;
	std::shared_ptr<Instance> FindFirstChildOfClass(const char *className, bool recursive = false) const;
	std::shared_ptr<Instance> FindFirstAncestor(const char *name) const;
	std::shared_ptr<Instance> FindFirstAncestorOfClass(const char *className) const;
	std::shared_ptr<Instance> FindFirstAncestorWhichIsA(const char *className) const;
	std::shared_ptr<Instance> FindFirstChildWhichIsA(const char *className, bool recursive = false) const;
	bool IsAncestorOf(const Instance *descendant) const;
	bool IsDescendantOf(const Instance *ancestor) const;
	std::string GetFullName() const;

	// Luau method implementations
	static int GetChildrenLuau(lua_State *L);
	static int GetDescendantsLuau(lua_State *L);
	static int FindFirstChildLuau(lua_State *L);
	static int FindFirstChildOfClassLuau(lua_State *L);
	static int FindFirstAncestorLuau(lua_State *L);
	static int FindFirstAncestorOfClassLuau(lua_State *L);
	static int FindFirstAncestorWhichIsALuau(lua_State *L);
	static int FindFirstChildWhichIsALuau(lua_State *L);
	static int IsAncestorOfLuau(lua_State *L);
	static int IsDescendantOfLuau(lua_State *L);

	// Destruction
	void Destroy();
	static int DestroyLuau(lua_State *L);

	// ClearAllChildren
	void ClearAllChildren();
	static int ClearAllChildrenLuau(lua_State *L);

	// Internal helpers for parent management
	void AddChild(std::shared_ptr<Instance> child);
	void RemoveChild(Instance *child);

	// Get self as shared_ptr (for use in SetParent)
	std::shared_ptr<Instance> GetSelf() const { return selfWeak.lock(); }
	void SetSelf(std::shared_ptr<Instance> self) { selfWeak = self; }

	// Check if destroyed
	bool IsDestroyed() const { return destroyed; }

protected:
	template <typename T>
	static void BindMembers() {
		Object::BindMembers<T>();

		// Properties
		ClassDB::BindProperty<T, "Name", "Data", &Instance::GetName, NoneSecurity,
				&Instance::SetName, NoneSecurity, ThreadSafety::Unsafe, true, true>({});

		// Parent is handled via index/newindex overrides

		// Methods
		ClassDB::BindLuauMethod<T, "GetChildren", std::vector<std::shared_ptr<Instance>>(),
				&T::GetChildrenLuau, NoneSecurity, ThreadSafety::Safe>({});

		ClassDB::BindLuauMethod<T, "GetDescendants", std::vector<std::shared_ptr<Instance>>(),
				&T::GetDescendantsLuau, NoneSecurity, ThreadSafety::Safe>({});

		ClassDB::BindLuauMethod<T, "FindFirstChild", std::shared_ptr<Instance>(const char *, std::optional<bool>),
				&T::FindFirstChildLuau, NoneSecurity, ThreadSafety::Safe>({}, "name", "recursive");

		ClassDB::BindLuauMethod<T, "FindFirstChildOfClass", std::shared_ptr<Instance>(const char *, std::optional<bool>),
				&T::FindFirstChildOfClassLuau, NoneSecurity, ThreadSafety::Safe>({}, "className", "recursive");

		ClassDB::BindLuauMethod<T, "FindFirstAncestor", std::shared_ptr<Instance>(const char *),
				&T::FindFirstAncestorLuau, NoneSecurity, ThreadSafety::Safe>({}, "name");

		ClassDB::BindLuauMethod<T, "FindFirstAncestorOfClass", std::shared_ptr<Instance>(const char *),
				&T::FindFirstAncestorOfClassLuau, NoneSecurity, ThreadSafety::Safe>({}, "className");

		ClassDB::BindLuauMethod<T, "FindFirstAncestorWhichIsA", std::shared_ptr<Instance>(const char *),
				&T::FindFirstAncestorWhichIsALuau, NoneSecurity, ThreadSafety::Safe>({}, "className");

		ClassDB::BindLuauMethod<T, "FindFirstChildWhichIsA", std::shared_ptr<Instance>(const char *, std::optional<bool>),
				&T::FindFirstChildWhichIsALuau, NoneSecurity, ThreadSafety::Safe>({}, "className", "recursive");

		ClassDB::BindLuauMethod<T, "IsAncestorOf", bool(std::shared_ptr<Instance>),
				&T::IsAncestorOfLuau, NoneSecurity, ThreadSafety::Safe>({}, "descendant");

		ClassDB::BindLuauMethod<T, "IsDescendantOf", bool(std::shared_ptr<Instance>),
				&T::IsDescendantOfLuau, NoneSecurity, ThreadSafety::Safe>({}, "ancestor");

		ClassDB::BindMethod<T, "GetFullName", &Instance::GetFullName, NoneSecurity, ThreadSafety::Safe>({});

		ClassDB::BindLuauMethod<T, "Destroy", void(),
				&T::DestroyLuau, NoneSecurity, ThreadSafety::Unsafe>({});

		ClassDB::BindLuauMethod<T, "ClearAllChildren", void(),
				&T::ClearAllChildrenLuau, NoneSecurity, ThreadSafety::Unsafe>({});

		// Signals
		ClassDB::BindSignal<T, "ChildAdded", void(std::shared_ptr<Instance>), NoneSecurity>({}, "child");
		ClassDB::BindSignal<T, "ChildRemoved", void(std::shared_ptr<Instance>), NoneSecurity>({}, "child");
		ClassDB::BindSignal<T, "DescendantAdded", void(std::shared_ptr<Instance>), NoneSecurity>({}, "descendant");
		ClassDB::BindSignal<T, "DescendantRemoving", void(std::shared_ptr<Instance>), NoneSecurity>({}, "descendant");
		ClassDB::BindSignal<T, "AncestryChanged", void(std::shared_ptr<Instance>, std::shared_ptr<Instance>), NoneSecurity>({}, "child", "parent");
		ClassDB::BindSignal<T, "Destroying", void(), NoneSecurity>({});

		// Register Parent property in ClassDB for reflection (not scriptable via normal path)
		ClassDB::BindPropertyNotScriptable<T, "Parent", "Data",
				&Instance::GetParent, &Instance::SetParent,
				ThreadSafety::Unsafe, true, true>({});

		// Add custom index/newindex overrides for Parent property
		LuauClassBinder<T>::AddIndexOverride(ParentIndexOverride);
		LuauClassBinder<T>::AddNewindexOverride(ParentNewindexOverride);
	}

private:
	std::string name = "Instance";
	std::weak_ptr<Instance> parent;
	std::vector<std::shared_ptr<Instance>> children;
	std::weak_ptr<Instance> selfWeak;
	bool destroyed = false;

	// Custom index/newindex overrides for Parent property
	static int ParentIndexOverride(lua_State *L, const char *propName);
	static bool ParentNewindexOverride(lua_State *L, const char *propName);

	// Helper to emit descendant signals up the tree
	void EmitDescendantAdded(std::shared_ptr<Instance> descendant);
	void EmitDescendantRemoving(std::shared_ptr<Instance> descendant);
	void EmitAncestryChanged(std::shared_ptr<Instance> child, std::shared_ptr<Instance> newParent);

	// Helper to collect descendants recursively
	void CollectDescendants(std::vector<std::shared_ptr<Instance>> &result) const;
};

} //namespace SBX::Classes
