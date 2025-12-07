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
#include <string>

#include "lua.h"

#include "Sbx/Classes/Instance.hpp"

namespace SBX::Classes {

/**
 * @brief This class implements a simplified version of Roblox's
 * [`Script`](https://create.roblox.com/docs/reference/engine/classes/Script) class.
 *
 * Script is a container for Luau code. When running, the script's object can be
 * accessed via the `script` global, and `script.Parent` gives the script's parent.
 */
class Script : public Instance {
	SBXCLASS(Script, Instance, MemoryCategory::Script);

public:
	Script();
	~Script() override = default;

	// Source property - the Luau source code
	const char *GetSource() const { return source.c_str(); }
	void SetSource(const char *code);

	// Disabled property - whether the script is disabled
	bool GetDisabled() const { return disabled; }
	void SetDisabled(bool value);

	// RunContext - where the script runs (Server, Client, Legacy)
	const char *GetRunContext() const { return runContext.c_str(); }
	void SetRunContext(const char *context);

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();

		// Properties
		ClassDB::BindProperty<T, "Source", "Data", &Script::GetSource, PluginSecurity,
				&Script::SetSource, PluginSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "Disabled", "Behavior", &Script::GetDisabled, NoneSecurity,
				&Script::SetDisabled, NoneSecurity, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindProperty<T, "RunContext", "Data", &Script::GetRunContext, NoneSecurity,
				&Script::SetRunContext, NoneSecurity, ThreadSafety::Safe, true, true>({});
	}

private:
	std::string source;
	bool disabled = false;
	std::string runContext = "Legacy";
};

/**
 * @brief LocalScript is a script that runs on the client.
 */
class LocalScript : public Script {
	SBXCLASS(LocalScript, Script, MemoryCategory::Script);

public:
	LocalScript();
	~LocalScript() override = default;

protected:
	template <typename T>
	static void BindMembers() {
		Script::BindMembers<T>();
	}
};

/**
 * @brief ModuleScript is a script that can be required by other scripts.
 */
class ModuleScript : public Instance {
	SBXCLASS(ModuleScript, Instance, MemoryCategory::Script);

public:
	ModuleScript();
	~ModuleScript() override = default;

	// Source property
	const char *GetSource() const { return source.c_str(); }
	void SetSource(const char *code);

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();

		ClassDB::BindProperty<T, "Source", "Data", &ModuleScript::GetSource, PluginSecurity,
				&ModuleScript::SetSource, PluginSecurity, ThreadSafety::Unsafe, true, true>({});
	}

private:
	std::string source;
};

} // namespace SBX::Classes
