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

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX {

class SignalEmitter;

}

namespace SBX::DataTypes {

/**
 * @brief This class implements Roblox's [`RBXScriptSignal`](https://create.roblox.com/docs/en-us/reference/engine/datatypes/RBXScriptSignal)
 * data type.
 */
class RBXScriptSignal {
public:
	RBXScriptSignal(); // required for stack operation
	RBXScriptSignal(std::shared_ptr<SignalEmitter> emitter, std::string name, SbxCapability security = NoneSecurity);

	static void Register(lua_State *L);

	static int Connect(lua_State *L);
	static int Once(lua_State *L);
	static int Wait(lua_State *L);

	std::string ToString() const;
	bool operator==(const RBXScriptSignal &other) const;

private:
	// Hold this reference to allow this object to continue working after the
	// emitter owner is collected.
	std::shared_ptr<SignalEmitter> emitter;
	std::string name;
	SbxCapability security = NoneSecurity;
};

} //namespace SBX::DataTypes

namespace SBX {

STACK_OP_UDATA_DEF(DataTypes::RBXScriptSignal);

}
