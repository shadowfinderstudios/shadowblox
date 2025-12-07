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

#include "SbxGD/RegisterTypes.hpp"

#include "gdextension_interface.h"

#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/godot.hpp"

#include "SbxGD/SbxRuntime.hpp"
#include "SbxGD/SbxInstance.hpp"
#include "SbxGD/SbxPart.hpp"
#include "SbxGD/SbxModel.hpp"

using namespace godot;

void initializeShadowbloxModule(ModuleInitializationLevel level) {
	if (level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// Register shadowblox wrapper classes
	ClassDB::register_class<SbxGD::SbxRuntime>();
	ClassDB::register_class<SbxGD::SbxInstance>();
	ClassDB::register_class<SbxGD::SbxPart>();
	ClassDB::register_class<SbxGD::SbxModel>();
}

void uninitializeShadowbloxModule(ModuleInitializationLevel level) {
	if (level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C" {
GDExtensionBool GDE_EXPORT shadowbloxgdInit(GDExtensionInterfaceGetProcAddress getProcAddress, GDExtensionClassLibraryPtr library, GDExtensionInitialization *initialization) {
	godot::GDExtensionBinding::InitObject initObj(getProcAddress, library, initialization);

	initObj.register_initializer(initializeShadowbloxModule);
	initObj.register_terminator(uninitializeShadowbloxModule);
	initObj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return initObj.init();
}
}
