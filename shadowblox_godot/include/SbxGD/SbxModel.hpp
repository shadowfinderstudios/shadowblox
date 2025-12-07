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

#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/core/class_db.hpp"

namespace SBX::Classes {
class Model;
class Part;
}

namespace SbxGD {

class SbxPart;

// SbxModel - Godot wrapper for shadowblox Model objects
// Groups multiple SbxPart children together
class SbxModel : public godot::Node3D {
	GDCLASS(SbxModel, godot::Node3D);

public:
	SbxModel();
	~SbxModel();

	// Godot lifecycle
	void _ready() override;

	// Bind to a shadowblox Model
	void bind_model(std::shared_ptr<SBX::Classes::Model> model);

	// Get the bound shadowblox Model
	std::shared_ptr<SBX::Classes::Model> get_sbx_model() const { return model; }

	// Property accessors for Godot
	godot::String get_sbx_name() const;
	void set_sbx_name(const godot::String &name);

	SbxPart *get_primary_part() const;
	void set_primary_part(SbxPart *part);

	// Model methods
	godot::Vector3 get_extents_size() const;
	void move_to(const godot::Vector3 &position);
	void translate_by(const godot::Vector3 &offset);

	// Create child SbxPart nodes for all Part children in the model
	void sync_children();

protected:
	static void _bind_methods();

private:
	std::shared_ptr<SBX::Classes::Model> model;
};

} // namespace SbxGD
