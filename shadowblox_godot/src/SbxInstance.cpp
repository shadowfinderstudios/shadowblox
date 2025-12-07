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

#include "SbxGD/SbxInstance.hpp"

#include "godot_cpp/variant/utility_functions.hpp"

#include "Sbx/Classes/Instance.hpp"

namespace SbxGD {

SbxInstance::SbxInstance() {
}

SbxInstance::~SbxInstance() {
	disconnect_signals();
}

void SbxInstance::bind_instance(std::shared_ptr<SBX::Classes::Instance> inst) {
	if (instance) {
		disconnect_signals();
	}

	instance = inst;

	if (instance) {
		// Set the Godot node name to match the instance name
		set_name(godot::String(instance->GetName()));
		connect_signals();
		_on_instance_bound();
	}
}

godot::String SbxInstance::get_sbx_name() const {
	if (!instance) {
		return "";
	}
	return godot::String(instance->GetName());
}

void SbxInstance::set_sbx_name(const godot::String &name) {
	if (!instance) {
		return;
	}
	instance->SetName(name.utf8().get_data());
	set_name(name);
}

godot::String SbxInstance::get_sbx_class_name() const {
	if (!instance) {
		return "";
	}
	return godot::String(instance->GetClassName());
}

godot::String SbxInstance::get_full_name() const {
	if (!instance) {
		return "";
	}
	return godot::String(instance->GetFullName().c_str());
}

godot::TypedArray<SbxInstance> SbxInstance::get_sbx_children() const {
	godot::TypedArray<SbxInstance> result;

	if (!instance) {
		return result;
	}

	// Get all child nodes that are SbxInstance
	for (int i = 0; i < get_child_count(); i++) {
		godot::Node *child = get_child(i);
		SbxInstance *sbx_child = godot::Object::cast_to<SbxInstance>(child);
		if (sbx_child) {
			result.append(sbx_child);
		}
	}

	return result;
}

SbxInstance *SbxInstance::find_first_child(const godot::String &name, bool recursive) const {
	if (!instance) {
		return nullptr;
	}

	std::shared_ptr<SBX::Classes::Instance> found = instance->FindFirstChild(name.utf8().get_data(), recursive);
	if (!found) {
		return nullptr;
	}

	// Search our Godot children for the matching SbxInstance
	for (int i = 0; i < get_child_count(); i++) {
		godot::Node *child = get_child(i);
		SbxInstance *sbx_child = godot::Object::cast_to<SbxInstance>(child);
		if (sbx_child && sbx_child->get_sbx_instance() == found) {
			return sbx_child;
		}

		if (recursive && sbx_child) {
			SbxInstance *result = sbx_child->find_first_child(name, true);
			if (result) {
				return result;
			}
		}
	}

	return nullptr;
}

void SbxInstance::destroy() {
	if (instance) {
		instance->Destroy();
	}
	queue_free();
}

bool SbxInstance::is_destroyed() const {
	if (!instance) {
		return true;
	}
	return instance->IsDestroyed();
}

void SbxInstance::connect_signals() {
	// Signal connections would go here
	// For now, we'll handle property changes through polling or manual sync
}

void SbxInstance::disconnect_signals() {
	// Disconnect signal handlers
}

void SbxInstance::_on_property_changed(const godot::String &property) {
	// Override in subclasses to handle specific property changes
	if (property == godot::String("Name")) {
		set_name(godot::String(instance->GetName()));
	}
}

void SbxInstance::_bind_methods() {
	// Properties
	godot::ClassDB::bind_method(godot::D_METHOD("get_sbx_name"), &SbxInstance::get_sbx_name);
	godot::ClassDB::bind_method(godot::D_METHOD("set_sbx_name", "name"), &SbxInstance::set_sbx_name);
	godot::ClassDB::add_property("SbxInstance",
			godot::PropertyInfo(godot::Variant::STRING, "sbx_name"),
			"set_sbx_name", "get_sbx_name");

	godot::ClassDB::bind_method(godot::D_METHOD("get_sbx_class_name"), &SbxInstance::get_sbx_class_name);
	godot::ClassDB::add_property("SbxInstance",
			godot::PropertyInfo(godot::Variant::STRING, "sbx_class_name", godot::PROPERTY_HINT_NONE, "", godot::PROPERTY_USAGE_READ_ONLY | godot::PROPERTY_USAGE_EDITOR),
			"", "get_sbx_class_name");

	godot::ClassDB::bind_method(godot::D_METHOD("get_full_name"), &SbxInstance::get_full_name);

	// Hierarchy methods
	godot::ClassDB::bind_method(godot::D_METHOD("get_sbx_children"), &SbxInstance::get_sbx_children);
	godot::ClassDB::bind_method(godot::D_METHOD("find_first_child", "name", "recursive"), &SbxInstance::find_first_child, DEFVAL(false));

	// Destruction
	godot::ClassDB::bind_method(godot::D_METHOD("destroy"), &SbxInstance::destroy);
	godot::ClassDB::bind_method(godot::D_METHOD("is_destroyed"), &SbxInstance::is_destroyed);
}

} // namespace SbxGD
