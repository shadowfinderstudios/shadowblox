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

#include "Sbx/DataTypes/RBXScriptConnection.hpp"

#include "lua.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/ClassBinder.hpp"
#include "Sbx/Runtime/SignalEmitter.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

RBXScriptConnection::RBXScriptConnection() :
		id(0) {
}

RBXScriptConnection::RBXScriptConnection(std::shared_ptr<SignalEmitter> emitter, std::string name, uint64_t id) :
		emitter(std::move(emitter)), name(std::move(name)), id(id) {
}

void RBXScriptConnection::Register(lua_State *L) {
	using B = LuauClassBinder<RBXScriptConnection>;

	if (!B::IsInitialized()) {
		B::Init("RBXScriptConnection", "RBXScriptConnection", RBXScriptConnectionUdata);

		B::BindPropertyReadOnly<"Connected", &RBXScriptConnection::IsConnected, NoneSecurity>();
		B::BindMethod<"Disconnect", &RBXScriptConnection::Disconnect, NoneSecurity>();

		B::BindToString<&RBXScriptConnection::ToString>();
	}

	B::InitMetatable(L);
}

bool RBXScriptConnection::IsConnected() const {
	return emitter->IsConnected(name, id);
}

void RBXScriptConnection::Disconnect() {
	emitter->Disconnect(name, id);
}

const char *RBXScriptConnection::ToString() const {
	return "Connection";
}

} //namespace SBX::DataTypes

namespace SBX {

using DataTypes::RBXScriptConnection;
UDATA_STACK_OP_IMPL(RBXScriptConnection, "RBXScriptConnection", "RBXScriptConnection", RBXScriptConnectionUdata, DTOR(RBXScriptConnection));

} //namespace SBX
