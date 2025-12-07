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

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/core/class_db.hpp"

namespace SBX::Classes {
class Instance;
}

namespace SbxGD {

// SbxInstance - Base Godot wrapper for shadowblox Instance objects
// Synchronizes the Godot node tree with the shadowblox instance tree
class SbxInstance : public godot::Node {
	GDCLASS(SbxInstance, godot::Node);

public:
	SbxInstance();
	~SbxInstance();

	// Bind to a shadowblox Instance
	void bind_instance(std::shared_ptr<SBX::Classes::Instance> inst);

	// Get the bound shadowblox instance
	std::shared_ptr<SBX::Classes::Instance> get_sbx_instance() const { return instance; }

	// Property accessors for Godot
	godot::String get_sbx_name() const;
	void set_sbx_name(const godot::String &name);

	godot::String get_sbx_class_name() const;
	godot::String get_full_name() const;

	// Hierarchy methods
	godot::TypedArray<SbxInstance> get_sbx_children() const;
	SbxInstance *find_first_child(const godot::String &name, bool recursive = false) const;

	// Destruction
	void destroy();
	bool is_destroyed() const;

protected:
	static void _bind_methods();

	// Called when the instance is bound - override in subclasses
	virtual void _on_instance_bound() {}

	// Called when a property changes on the shadowblox side
	virtual void _on_property_changed(const godot::String &property);

	std::shared_ptr<SBX::Classes::Instance> instance;

private:
	void connect_signals();
	void disconnect_signals();
};

} // namespace SbxGD
