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

namespace SBX {
class LuauRuntime;
}

namespace SbxGD {

// SbxRuntime - Manages the Luau runtime within Godot
// Add as a child of your scene to enable script execution
class SbxRuntime : public godot::Node {
	GDCLASS(SbxRuntime, godot::Node);

public:
	SbxRuntime();
	~SbxRuntime();

	// Godot lifecycle
	void _ready() override;
	void _process(double delta) override;

	// Runtime access
	SBX::LuauRuntime *get_runtime() const { return runtime.get(); }

	// Execute a Luau script
	godot::String execute_script(const godot::String &code);

	// GC control
	void gc_step(int step_size);
	int get_gc_memory() const;

protected:
	static void _bind_methods();

private:
	std::unique_ptr<SBX::LuauRuntime> runtime;
	bool initialized = false;

	void initialize_runtime();
};

} // namespace SbxGD
