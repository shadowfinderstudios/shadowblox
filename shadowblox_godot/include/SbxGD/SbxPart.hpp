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

#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/box_mesh.hpp"
#include "godot_cpp/classes/box_shape3d.hpp"
#include "godot_cpp/classes/area3d.hpp"
#include "godot_cpp/classes/collision_shape3d.hpp"
#include "godot_cpp/classes/standard_material3d.hpp"
#include "godot_cpp/core/class_db.hpp"

namespace SBX::Classes {
class Part;
}

namespace SbxGD {

// SbxPart - Godot wrapper for shadowblox Part objects
// Renders as a 3D box mesh synchronized with Part properties
class SbxPart : public godot::MeshInstance3D {
	GDCLASS(SbxPart, godot::MeshInstance3D);

public:
	SbxPart();
	~SbxPart();

	// Godot lifecycle
	void _ready() override;
	void _process(double delta) override;

	// Bind to a shadowblox Part
	void bind_part(std::shared_ptr<SBX::Classes::Part> part);

	// Get the bound shadowblox Part
	std::shared_ptr<SBX::Classes::Part> get_sbx_part() const { return part; }

	// Property accessors for Godot
	godot::String get_sbx_name() const;
	void set_sbx_name(const godot::String &name);

	godot::Vector3 get_sbx_size() const;
	void set_sbx_size(const godot::Vector3 &size);

	godot::Vector3 get_sbx_position() const;
	void set_sbx_position(const godot::Vector3 &pos);

	bool get_anchored() const;
	void set_anchored(bool anchored);

	bool get_can_collide() const;
	void set_can_collide(bool can_collide);

	double get_transparency() const;
	void set_transparency(double transparency);

	bool get_can_touch() const;
	void set_can_touch(bool can_touch);

	// Sync all properties from shadowblox to Godot
	void sync_from_sbx();

	// Sync all properties from Godot to shadowblox
	void sync_to_sbx();

	// Collision callbacks
	void _on_area_entered(godot::Area3D *area);
	void _on_area_exited(godot::Area3D *area);

protected:
	static void _bind_methods();

private:
	std::shared_ptr<SBX::Classes::Part> part;
	godot::Ref<godot::BoxMesh> box_mesh;
	godot::Ref<godot::StandardMaterial3D> material;

	// Collision detection
	godot::Area3D *collision_area = nullptr;
	godot::CollisionShape3D *collision_shape = nullptr;
	godot::Ref<godot::BoxShape3D> box_shape;

	void setup_mesh();
	void setup_collision();
	void update_mesh_size();
	void update_collision_shape();
	void update_material();
};

} // namespace SbxGD
