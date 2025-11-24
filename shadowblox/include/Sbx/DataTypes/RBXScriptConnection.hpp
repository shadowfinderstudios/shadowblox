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

#include <cstdint>
#include <memory>
#include <string>

#include "lua.h"

#include "Sbx/Runtime/Stack.hpp"

namespace SBX {

class SignalEmitter;

}

namespace SBX::DataTypes {

/**
 * @brief This class implements Roblox's [`RBXScriptConnection`](https://create.roblox.com/docs/en-us/reference/engine/datatypes/RBXScriptConnection)
 * data type.
 */
class RBXScriptConnection {
public:
	RBXScriptConnection(); // required for stack operation
	RBXScriptConnection(std::shared_ptr<SignalEmitter> emitter, std::string name, uint64_t id);

	static void Register(lua_State *L);

	bool IsConnected() const;
	void Disconnect();

	const char *ToString() const;

private:
	// Hold this reference to force connections to stay connected once the
	// emitter owner is collected.
	std::shared_ptr<SignalEmitter> emitter;
	std::string name;
	uint64_t id;
};

} //namespace SBX::DataTypes

namespace SBX {

STACK_OP_UDATA_DEF(DataTypes::RBXScriptConnection);

}
