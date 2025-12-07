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

#include "SbxGD/SbxModel.hpp"

#include "godot_cpp/variant/utility_functions.hpp"

#include "SbxGD/SbxPart.hpp"
#include "Sbx/Classes/Model.hpp"
#include "Sbx/Classes/Part.hpp"
#include "Sbx/DataTypes/Vector3.hpp"
#include "Sbx/GodotBridge.hpp"

namespace SbxGD {

SbxModel::SbxModel() {
}

SbxModel::~SbxModel() {
}

void SbxModel::_ready() {
	sync_children();
}

void SbxModel::bind_model(std::shared_ptr<SBX::Classes::Model> m) {
	model = m;

	if (model) {
		set_name(godot::String(model->GetName()));
		sync_children();
	}
}

godot::String SbxModel::get_sbx_name() const {
	if (!model) {
		return "";
	}
	return godot::String(model->GetName());
}

void SbxModel::set_sbx_name(const godot::String &name) {
	if (!model) {
		return;
	}
	model->SetName(name.utf8().get_data());
	set_name(name);
}

SbxPart *SbxModel::get_primary_part() const {
	if (!model) {
		return nullptr;
	}

	std::shared_ptr<SBX::Classes::Part> primary = model->GetPrimaryPart();
	if (!primary) {
		return nullptr;
	}

	// Find the matching SbxPart child
	for (int i = 0; i < get_child_count(); i++) {
		godot::Node *child = get_child(i);
		SbxPart *sbx_part = godot::Object::cast_to<SbxPart>(child);
		if (sbx_part && sbx_part->get_sbx_part() == primary) {
			return sbx_part;
		}
	}

	return nullptr;
}

void SbxModel::set_primary_part(SbxPart *part) {
	if (!model) {
		return;
	}

	if (part && part->get_sbx_part()) {
		model->SetPrimaryPart(part->get_sbx_part());
	} else {
		model->SetPrimaryPart(nullptr);
	}
}

godot::Vector3 SbxModel::get_extents_size() const {
	if (!model) {
		return godot::Vector3();
	}

	SBX::DataTypes::Vector3 size = model->GetExtentsSize();
	return godot::Vector3(size.X, size.Y, size.Z);
}

void SbxModel::move_to(const godot::Vector3 &position) {
	if (!model) {
		return;
	}

	model->MoveTo(SBX::DataTypes::Vector3(position.x, position.y, position.z));

	// Sync all child parts
	for (int i = 0; i < get_child_count(); i++) {
		godot::Node *child = get_child(i);
		SbxPart *sbx_part = godot::Object::cast_to<SbxPart>(child);
		if (sbx_part) {
			sbx_part->sync_from_sbx();
		}
	}
}

void SbxModel::translate_by(const godot::Vector3 &offset) {
	if (!model) {
		return;
	}

	model->TranslateBy(SBX::DataTypes::Vector3(offset.x, offset.y, offset.z));

	// Sync all child parts
	for (int i = 0; i < get_child_count(); i++) {
		godot::Node *child = get_child(i);
		SbxPart *sbx_part = godot::Object::cast_to<SbxPart>(child);
		if (sbx_part) {
			sbx_part->sync_from_sbx();
		}
	}
}

void SbxModel::sync_children() {
	if (!model) {
		return;
	}

	// Get all Part children from the shadowblox model
	auto children = model->GetChildren();

	for (const auto &child : children) {
		// Check if this is a Part
		SBX::Classes::Part *part_ptr = dynamic_cast<SBX::Classes::Part *>(child.get());
		if (part_ptr) {
			// Check if we already have a SbxPart for this
			bool found = false;
			for (int i = 0; i < get_child_count(); i++) {
				SbxPart *sbx_part = godot::Object::cast_to<SbxPart>(get_child(i));
				if (sbx_part && sbx_part->get_sbx_part().get() == part_ptr) {
					found = true;
					break;
				}
			}

			if (!found) {
				// Create a new SbxPart
				SbxPart *sbx_part = memnew(SbxPart);
				sbx_part->bind_part(std::dynamic_pointer_cast<SBX::Classes::Part>(child));
				add_child(sbx_part);
			}
		}

		// Check if this is a Model (nested)
		SBX::Classes::Model *model_ptr = dynamic_cast<SBX::Classes::Model *>(child.get());
		if (model_ptr) {
			// Check if we already have a SbxModel for this
			bool found = false;
			for (int i = 0; i < get_child_count(); i++) {
				SbxModel *sbx_model = godot::Object::cast_to<SbxModel>(get_child(i));
				if (sbx_model && sbx_model->get_sbx_model().get() == model_ptr) {
					found = true;
					break;
				}
			}

			if (!found) {
				// Create a new SbxModel
				SbxModel *sbx_model = memnew(SbxModel);
				sbx_model->bind_model(std::dynamic_pointer_cast<SBX::Classes::Model>(child));
				add_child(sbx_model);
			}
		}
	}
}

void SbxModel::_bind_methods() {
	// Name property
	godot::ClassDB::bind_method(godot::D_METHOD("get_sbx_name"), &SbxModel::get_sbx_name);
	godot::ClassDB::bind_method(godot::D_METHOD("set_sbx_name", "name"), &SbxModel::set_sbx_name);
	godot::ClassDB::add_property("SbxModel",
			godot::PropertyInfo(godot::Variant::STRING, "sbx_name"),
			"set_sbx_name", "get_sbx_name");

	// PrimaryPart property
	godot::ClassDB::bind_method(godot::D_METHOD("get_primary_part"), &SbxModel::get_primary_part);
	godot::ClassDB::bind_method(godot::D_METHOD("set_primary_part", "part"), &SbxModel::set_primary_part);

	// Model methods
	godot::ClassDB::bind_method(godot::D_METHOD("get_extents_size"), &SbxModel::get_extents_size);
	godot::ClassDB::bind_method(godot::D_METHOD("move_to", "position"), &SbxModel::move_to);
	godot::ClassDB::bind_method(godot::D_METHOD("translate_by", "offset"), &SbxModel::translate_by);
	godot::ClassDB::bind_method(godot::D_METHOD("sync_children"), &SbxModel::sync_children);
}

} // namespace SbxGD
