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

#include "Sbx/Classes/Script.hpp"

namespace SBX::Classes {

Script::Script() :
		Instance() {
	SetName("Script");
}

void Script::SetSource(const char *code) {
	source = code ? code : "";
	Changed<Script>("Source");
}

void Script::SetDisabled(bool value) {
	disabled = value;
	Changed<Script>("Disabled");
}

void Script::SetRunContext(const char *context) {
	runContext = context ? context : "Legacy";
	Changed<Script>("RunContext");
}

LocalScript::LocalScript() :
		Script() {
	SetName("LocalScript");
	SetRunContext("Client");
}

ModuleScript::ModuleScript() :
		Instance() {
	SetName("ModuleScript");
}

void ModuleScript::SetSource(const char *code) {
	source = code ? code : "";
	Changed<ModuleScript>("Source");
}

} // namespace SBX::Classes
