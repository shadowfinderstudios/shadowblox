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

#include "Sbx/Classes/Instance.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Classes/ClassDB.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::Classes {

// Constructor
Instance::Instance() :
		Object() {
}

// Destructor
Instance::~Instance() {
	// When destroyed, detach from parent (if not already)
	if (!destroyed) {
		Destroy();
	}
}

// Properties
const char *Instance::GetName() const {
	return name.c_str();
}

void Instance::SetName(const char *newName) {
	if (destroyed) {
		return;
	}
	name = newName ? newName : "";
	Changed<Instance>("Name");
}

std::shared_ptr<Instance> Instance::GetParent() const {
	return parent.lock();
}

void Instance::SetParent(std::shared_ptr<Instance> newParent) {
	if (destroyed) {
		return;
	}

	std::shared_ptr<Instance> self = GetSelf();
	if (!self) {
		return;
	}

	std::shared_ptr<Instance> oldParent = parent.lock();

	// No change
	if (oldParent == newParent) {
		return;
	}

	// Prevent circular parenting
	if (newParent && IsAncestorOf(newParent.get())) {
		return;
	}

	// Remove from old parent
	if (oldParent) {
		oldParent->RemoveChild(this);
	}

	// Set new parent
	parent = newParent;

	// Add to new parent
	if (newParent) {
		newParent->AddChild(self);
	}

	// Emit signals
	EmitAncestryChanged(self, newParent);
}

// Internal child management
void Instance::AddChild(std::shared_ptr<Instance> child) {
	if (!child || destroyed) {
		return;
	}

	children.push_back(child);
	Emit<Instance>("ChildAdded", child);
	EmitDescendantAdded(child);

	// Also emit DescendantAdded for all descendants of the child
	std::vector<std::shared_ptr<Instance>> descendants = child->GetDescendants();
	for (const auto &descendant : descendants) {
		EmitDescendantAdded(descendant);
	}
}

void Instance::RemoveChild(Instance *child) {
	if (!child || destroyed) {
		return;
	}

	auto it = std::find_if(children.begin(), children.end(),
			[child](const std::shared_ptr<Instance> &c) { return c.get() == child; });

	if (it != children.end()) {
		std::shared_ptr<Instance> childPtr = *it;

		// Emit DescendantRemoving for all descendants first
		std::vector<std::shared_ptr<Instance>> descendants = childPtr->GetDescendants();
		for (auto rit = descendants.rbegin(); rit != descendants.rend(); ++rit) {
			EmitDescendantRemoving(*rit);
		}
		EmitDescendantRemoving(childPtr);

		children.erase(it);
		Emit<Instance>("ChildRemoved", childPtr);
	}
}

// Methods
std::vector<std::shared_ptr<Instance>> Instance::GetChildren() const {
	return children;
}

std::vector<std::shared_ptr<Instance>> Instance::GetDescendants() const {
	std::vector<std::shared_ptr<Instance>> result;
	CollectDescendants(result);
	return result;
}

void Instance::CollectDescendants(std::vector<std::shared_ptr<Instance>> &result) const {
	for (const auto &child : children) {
		result.push_back(child);
		child->CollectDescendants(result);
	}
}

std::shared_ptr<Instance> Instance::FindFirstChild(const char *searchName, bool recursive) const {
	for (const auto &child : children) {
		if (std::strcmp(child->GetName(), searchName) == 0) {
			return child;
		}
		if (recursive) {
			auto found = child->FindFirstChild(searchName, true);
			if (found) {
				return found;
			}
		}
	}
	return nullptr;
}

std::shared_ptr<Instance> Instance::FindFirstChildOfClass(const char *className, bool recursive) const {
	for (const auto &child : children) {
		if (std::strcmp(child->GetClassName(), className) == 0) {
			return child;
		}
		if (recursive) {
			auto found = child->FindFirstChildOfClass(className, true);
			if (found) {
				return found;
			}
		}
	}
	return nullptr;
}

std::shared_ptr<Instance> Instance::FindFirstAncestor(const char *searchName) const {
	std::shared_ptr<Instance> p = parent.lock();
	while (p) {
		if (std::strcmp(p->GetName(), searchName) == 0) {
			return p;
		}
		p = p->GetParent();
	}
	return nullptr;
}

std::shared_ptr<Instance> Instance::FindFirstAncestorOfClass(const char *className) const {
	std::shared_ptr<Instance> p = parent.lock();
	while (p) {
		if (std::strcmp(p->GetClassName(), className) == 0) {
			return p;
		}
		p = p->GetParent();
	}
	return nullptr;
}

std::shared_ptr<Instance> Instance::FindFirstAncestorWhichIsA(const char *className) const {
	std::shared_ptr<Instance> p = parent.lock();
	while (p) {
		if (p->IsA(className)) {
			return p;
		}
		p = p->GetParent();
	}
	return nullptr;
}

std::shared_ptr<Instance> Instance::FindFirstChildWhichIsA(const char *className, bool recursive) const {
	for (const auto &child : children) {
		if (child->IsA(className)) {
			return child;
		}
		if (recursive) {
			auto found = child->FindFirstChildWhichIsA(className, true);
			if (found) {
				return found;
			}
		}
	}
	return nullptr;
}

bool Instance::IsAncestorOf(const Instance *descendant) const {
	if (!descendant) {
		return false;
	}

	std::shared_ptr<Instance> p = descendant->GetParent();
	while (p) {
		if (p.get() == this) {
			return true;
		}
		p = p->GetParent();
	}
	return false;
}

bool Instance::IsDescendantOf(const Instance *ancestor) const {
	if (!ancestor) {
		return false;
	}

	return ancestor->IsAncestorOf(this);
}

std::string Instance::GetFullName() const {
	std::vector<std::string> names;
	names.push_back(name);

	std::shared_ptr<Instance> p = parent.lock();
	while (p) {
		names.push_back(p->GetName());
		p = p->GetParent();
	}

	std::ostringstream ss;
	for (auto it = names.rbegin(); it != names.rend(); ++it) {
		if (it != names.rbegin()) {
			ss << ".";
		}
		ss << *it;
	}
	return ss.str();
}

// Destruction
void Instance::Destroy() {
	if (destroyed) {
		return;
	}

	destroyed = true;
	Emit<Instance>("Destroying");

	// Clear all children first
	ClearAllChildren();

	// Remove from parent
	std::shared_ptr<Instance> p = parent.lock();
	if (p) {
		p->RemoveChild(this);
		parent.reset();
	}
}

void Instance::ClearAllChildren() {
	// Destroy all children (make copy to avoid iterator invalidation)
	std::vector<std::shared_ptr<Instance>> childrenCopy = children;
	for (auto &child : childrenCopy) {
		child->Destroy();
	}
	children.clear();
}

// Signal helpers
void Instance::EmitDescendantAdded(std::shared_ptr<Instance> descendant) {
	Emit<Instance>("DescendantAdded", descendant);

	std::shared_ptr<Instance> p = parent.lock();
	if (p) {
		p->EmitDescendantAdded(descendant);
	}
}

void Instance::EmitDescendantRemoving(std::shared_ptr<Instance> descendant) {
	Emit<Instance>("DescendantRemoving", descendant);

	std::shared_ptr<Instance> p = parent.lock();
	if (p) {
		p->EmitDescendantRemoving(descendant);
	}
}

void Instance::EmitAncestryChanged(std::shared_ptr<Instance> child, std::shared_ptr<Instance> newParent) {
	Emit<Instance>("AncestryChanged", child, newParent);

	// Also emit for all descendants
	for (const auto &c : children) {
		c->EmitAncestryChanged(c, newParent);
	}
}

// Custom index/newindex overrides for Parent property
int Instance::ParentIndexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "Parent") == 0) {
		Instance *self = LuauStackOp<Instance *>::Check(L, 1);
		std::shared_ptr<Instance> p = self->GetParent();
		if (p) {
			LuauStackOp<std::shared_ptr<Instance>>::Push(L, p);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}
	return 0; // Not handled
}

bool Instance::ParentNewindexOverride(lua_State *L, const char *propName) {
	if (std::strcmp(propName, "Parent") == 0) {
		Instance *self = LuauStackOp<Instance *>::Check(L, 1);

		std::shared_ptr<Instance> newParent;
		if (!lua_isnil(L, 3)) {
			newParent = LuauStackOp<std::shared_ptr<Instance>>::Check(L, 3);
		}

		self->SetParent(newParent);
		return true;
	}
	return false; // Not handled
}

// Luau method implementations
int Instance::GetChildrenLuau(lua_State *L) {
	Instance *self = LuauStackOp<Instance *>::Check(L, 1);
	std::vector<std::shared_ptr<Instance>> children = self->GetChildren();

	lua_createtable(L, children.size(), 0);
	for (size_t i = 0; i < children.size(); ++i) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, children[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

int Instance::GetDescendantsLuau(lua_State *L) {
	Instance *self = LuauStackOp<Instance *>::Check(L, 1);
	std::vector<std::shared_ptr<Instance>> descendants = self->GetDescendants();

	lua_createtable(L, descendants.size(), 0);
	for (size_t i = 0; i < descendants.size(); ++i) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, descendants[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

int Instance::FindFirstChildLuau(lua_State *L) {
	Instance *self = LuauStackOp<Instance *>::Check(L, 1);
	const char *searchName = luaL_checkstring(L, 2);
	bool recursive = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : false;

	std::shared_ptr<Instance> result = self->FindFirstChild(searchName, recursive);
	if (result) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, result);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Instance::FindFirstChildOfClassLuau(lua_State *L) {
	Instance *self = LuauStackOp<Instance *>::Check(L, 1);
	const char *className = luaL_checkstring(L, 2);
	bool recursive = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : false;

	std::shared_ptr<Instance> result = self->FindFirstChildOfClass(className, recursive);
	if (result) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, result);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Instance::FindFirstAncestorLuau(lua_State *L) {
	Instance *self = LuauStackOp<Instance *>::Check(L, 1);
	const char *searchName = luaL_checkstring(L, 2);

	std::shared_ptr<Instance> result = self->FindFirstAncestor(searchName);
	if (result) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, result);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Instance::FindFirstAncestorOfClassLuau(lua_State *L) {
	Instance *self = LuauStackOp<Instance *>::Check(L, 1);
	const char *className = luaL_checkstring(L, 2);

	std::shared_ptr<Instance> result = self->FindFirstAncestorOfClass(className);
	if (result) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, result);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Instance::FindFirstAncestorWhichIsALuau(lua_State *L) {
	Instance *self = LuauStackOp<Instance *>::Check(L, 1);
	const char *className = luaL_checkstring(L, 2);

	std::shared_ptr<Instance> result = self->FindFirstAncestorWhichIsA(className);
	if (result) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, result);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Instance::FindFirstChildWhichIsALuau(lua_State *L) {
	Instance *self = LuauStackOp<Instance *>::Check(L, 1);
	const char *className = luaL_checkstring(L, 2);
	bool recursive = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : false;

	std::shared_ptr<Instance> result = self->FindFirstChildWhichIsA(className, recursive);
	if (result) {
		LuauStackOp<std::shared_ptr<Instance>>::Push(L, result);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Instance::IsAncestorOfLuau(lua_State *L) {
	Instance *self = LuauStackOp<Instance *>::Check(L, 1);
	Instance *descendant = nullptr;

	if (!lua_isnil(L, 2)) {
		descendant = LuauStackOp<Instance *>::Check(L, 2);
	}

	lua_pushboolean(L, self->IsAncestorOf(descendant));
	return 1;
}

int Instance::IsDescendantOfLuau(lua_State *L) {
	Instance *self = LuauStackOp<Instance *>::Check(L, 1);
	Instance *ancestor = nullptr;

	if (!lua_isnil(L, 2)) {
		ancestor = LuauStackOp<Instance *>::Check(L, 2);
	}

	lua_pushboolean(L, self->IsDescendantOf(ancestor));
	return 1;
}

int Instance::DestroyLuau(lua_State *L) {
	Instance *self = LuauStackOp<Instance *>::Check(L, 1);
	self->Destroy();
	return 0;
}

int Instance::ClearAllChildrenLuau(lua_State *L) {
	Instance *self = LuauStackOp<Instance *>::Check(L, 1);
	self->ClearAllChildren();
	return 0;
}

} //namespace SBX::Classes
