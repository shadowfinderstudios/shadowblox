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

#include "SbxGD/SbxPart.hpp"

#include "godot_cpp/variant/utility_functions.hpp"

#include "Sbx/Classes/Part.hpp"
#include "Sbx/DataTypes/Vector3.hpp"
#include "Sbx/GodotBridge.hpp"

namespace SbxGD {

SbxPart::SbxPart() {
}

SbxPart::~SbxPart() {
}

void SbxPart::_ready() {
	setup_mesh();
	sync_from_sbx();
}

void SbxPart::_process(double delta) {
	// Sync from shadowblox each frame (can be optimized with signals later)
	sync_from_sbx();
}

void SbxPart::bind_part(std::shared_ptr<SBX::Classes::Part> p) {
	part = p;

	if (part) {
		set_name(godot::String(part->GetName()));
		sync_from_sbx();
	}
}

void SbxPart::setup_mesh() {
	// Create box mesh
	box_mesh.instantiate();
	set_mesh(box_mesh);

	// Create material for transparency
	material.instantiate();
	material->set_shading_mode(godot::StandardMaterial3D::SHADING_MODE_PER_PIXEL);
	set_surface_override_material(0, material);
}

void SbxPart::update_mesh_size() {
	if (!box_mesh.is_valid()) {
		return;
	}

	godot::Vector3 size = get_sbx_size();
	box_mesh->set_size(size);
}

void SbxPart::update_material() {
	if (!material.is_valid()) {
		return;
	}

	double trans = get_transparency();
	if (trans > 0.0) {
		material->set_transparency(godot::StandardMaterial3D::TRANSPARENCY_ALPHA);
		godot::Color color = material->get_albedo();
		color.a = 1.0 - trans;
		material->set_albedo(color);
	} else {
		material->set_transparency(godot::StandardMaterial3D::TRANSPARENCY_DISABLED);
	}
}

godot::String SbxPart::get_sbx_name() const {
	if (!part) {
		return "";
	}
	return godot::String(part->GetName());
}

void SbxPart::set_sbx_name(const godot::String &name) {
	if (!part) {
		return;
	}
	part->SetName(name.utf8().get_data());
	set_name(name);
}

godot::Vector3 SbxPart::get_sbx_size() const {
	if (!part) {
		return godot::Vector3(2.0, 1.0, 4.0); // Default Part size
	}
	SBX::DataTypes::Vector3 size = part->GetSize();
	return godot::Vector3(size.X, size.Y, size.Z);
}

void SbxPart::set_sbx_size(const godot::Vector3 &size) {
	if (!part) {
		return;
	}
	part->SetSize(SBX::DataTypes::Vector3(size.x, size.y, size.z));
	update_mesh_size();
}

godot::Vector3 SbxPart::get_sbx_position() const {
	if (!part) {
		return godot::Vector3();
	}
	SBX::DataTypes::Vector3 pos = part->GetPosition();
	return godot::Vector3(pos.X, pos.Y, pos.Z);
}

void SbxPart::set_sbx_position(const godot::Vector3 &pos) {
	if (!part) {
		return;
	}
	part->SetPosition(SBX::DataTypes::Vector3(pos.x, pos.y, pos.z));
	set_position(pos);
}

bool SbxPart::get_anchored() const {
	if (!part) {
		return false;
	}
	return part->GetAnchored();
}

void SbxPart::set_anchored(bool anchored) {
	if (!part) {
		return;
	}
	part->SetAnchored(anchored);
}

bool SbxPart::get_can_collide() const {
	if (!part) {
		return true;
	}
	return part->GetCanCollide();
}

void SbxPart::set_can_collide(bool can_collide) {
	if (!part) {
		return;
	}
	part->SetCanCollide(can_collide);
}

double SbxPart::get_transparency() const {
	if (!part) {
		return 0.0;
	}
	return part->GetTransparency();
}

void SbxPart::set_transparency(double transparency) {
	if (!part) {
		return;
	}
	part->SetTransparency(transparency);
	update_material();
}

bool SbxPart::get_can_touch() const {
	if (!part) {
		return true;
	}
	return part->GetCanTouch();
}

void SbxPart::set_can_touch(bool can_touch) {
	if (!part) {
		return;
	}
	part->SetCanTouch(can_touch);
}

void SbxPart::sync_from_sbx() {
	if (!part) {
		return;
	}

	// Sync position
	SBX::DataTypes::Vector3 pos = part->GetPosition();
	set_position(godot::Vector3(pos.X, pos.Y, pos.Z));

	// Sync size
	update_mesh_size();

	// Sync material (transparency)
	update_material();
}

void SbxPart::sync_to_sbx() {
	if (!part) {
		return;
	}

	// Sync position from Godot to shadowblox
	godot::Vector3 pos = get_position();
	part->SetPosition(SBX::DataTypes::Vector3(pos.x, pos.y, pos.z));
}

void SbxPart::_bind_methods() {
	// Name property
	godot::ClassDB::bind_method(godot::D_METHOD("get_sbx_name"), &SbxPart::get_sbx_name);
	godot::ClassDB::bind_method(godot::D_METHOD("set_sbx_name", "name"), &SbxPart::set_sbx_name);
	godot::ClassDB::add_property("SbxPart",
			godot::PropertyInfo(godot::Variant::STRING, "sbx_name"),
			"set_sbx_name", "get_sbx_name");

	// Size property
	godot::ClassDB::bind_method(godot::D_METHOD("get_sbx_size"), &SbxPart::get_sbx_size);
	godot::ClassDB::bind_method(godot::D_METHOD("set_sbx_size", "size"), &SbxPart::set_sbx_size);
	godot::ClassDB::add_property("SbxPart",
			godot::PropertyInfo(godot::Variant::VECTOR3, "sbx_size"),
			"set_sbx_size", "get_sbx_size");

	// Position property
	godot::ClassDB::bind_method(godot::D_METHOD("get_sbx_position"), &SbxPart::get_sbx_position);
	godot::ClassDB::bind_method(godot::D_METHOD("set_sbx_position", "position"), &SbxPart::set_sbx_position);
	godot::ClassDB::add_property("SbxPart",
			godot::PropertyInfo(godot::Variant::VECTOR3, "sbx_position"),
			"set_sbx_position", "get_sbx_position");

	// Anchored property
	godot::ClassDB::bind_method(godot::D_METHOD("get_anchored"), &SbxPart::get_anchored);
	godot::ClassDB::bind_method(godot::D_METHOD("set_anchored", "anchored"), &SbxPart::set_anchored);
	godot::ClassDB::add_property("SbxPart",
			godot::PropertyInfo(godot::Variant::BOOL, "anchored"),
			"set_anchored", "get_anchored");

	// CanCollide property
	godot::ClassDB::bind_method(godot::D_METHOD("get_can_collide"), &SbxPart::get_can_collide);
	godot::ClassDB::bind_method(godot::D_METHOD("set_can_collide", "can_collide"), &SbxPart::set_can_collide);
	godot::ClassDB::add_property("SbxPart",
			godot::PropertyInfo(godot::Variant::BOOL, "can_collide"),
			"set_can_collide", "get_can_collide");

	// Transparency property
	godot::ClassDB::bind_method(godot::D_METHOD("get_transparency"), &SbxPart::get_transparency);
	godot::ClassDB::bind_method(godot::D_METHOD("set_transparency", "transparency"), &SbxPart::set_transparency);
	godot::ClassDB::add_property("SbxPart",
			godot::PropertyInfo(godot::Variant::FLOAT, "transparency", godot::PROPERTY_HINT_RANGE, "0,1,0.01"),
			"set_transparency", "get_transparency");

	// CanTouch property
	godot::ClassDB::bind_method(godot::D_METHOD("get_can_touch"), &SbxPart::get_can_touch);
	godot::ClassDB::bind_method(godot::D_METHOD("set_can_touch", "can_touch"), &SbxPart::set_can_touch);
	godot::ClassDB::add_property("SbxPart",
			godot::PropertyInfo(godot::Variant::BOOL, "can_touch"),
			"set_can_touch", "get_can_touch");

	// Sync methods
	godot::ClassDB::bind_method(godot::D_METHOD("sync_from_sbx"), &SbxPart::sync_from_sbx);
	godot::ClassDB::bind_method(godot::D_METHOD("sync_to_sbx"), &SbxPart::sync_to_sbx);
}

} // namespace SbxGD
