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

#include "Sbx/DataTypes/RBXScriptSignal.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "ltm.h"
#include "lua.h"
#include "lualib.h"

#include "Sbx/DataTypes/RBXScriptConnection.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/ClassBinder.hpp"
#include "Sbx/Runtime/SignalEmitter.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

RBXScriptSignal::RBXScriptSignal() {
}

RBXScriptSignal::RBXScriptSignal(std::shared_ptr<SignalEmitter> emitter, std::string name, SbxCapability security) :
		emitter(std::move(emitter)), name(std::move(name)), security(security) {
}

void RBXScriptSignal::Register(lua_State *L) {
	using B = LuauClassBinder<RBXScriptSignal>;

	if (!B::IsInitialized()) {
		B::Init("RBXScriptSignal", "RBXScriptSignal", RBXScriptSignalUdata);

		B::BindLuauMethod<"Connect", RBXScriptSignal::Connect>();
		B::BindLuauMethod<"Once", RBXScriptSignal::Once>();
		B::BindLuauMethod<"Wait", RBXScriptSignal::Wait>();

		B::BindToString<&RBXScriptSignal::ToString>();
		B::BindBinaryOp<TM_EQ, &RBXScriptSignal::operator== >(LuauStackOp<RBXScriptSignal>::Is, LuauStackOp<RBXScriptSignal>::Is);
	}

	B::InitMetatable(L);
}

int RBXScriptSignal::Connect(lua_State *L) {
	RBXScriptSignal *self = LuauStackOp<RBXScriptSignal *>::Check(L, 1);
	if (!lua_isfunction(L, 2)) {
		luaL_error(L, "Attempt to connect failed: Passed value is not a function");
	}

	luaSBX_checkcapability(L, self->security, "connect", self->name.c_str());

	uint64_t id = self->emitter->Connect(self->name, L, false);
	LuauStackOp<RBXScriptConnection>::Push(L, RBXScriptConnection(self->emitter, self->name, id));
	return 1;
}

int RBXScriptSignal::Once(lua_State *L) {
	RBXScriptSignal *self = LuauStackOp<RBXScriptSignal *>::Check(L, 1);
	if (!lua_isfunction(L, 2)) {
		luaL_error(L, "Attempt to connect failed: Passed value is not a function");
	}

	luaSBX_checkcapability(L, self->security, "connect", self->name.c_str());

	uint64_t id = self->emitter->Connect(self->name, L, true);
	LuauStackOp<RBXScriptConnection>::Push(L, RBXScriptConnection(self->emitter, self->name, id));
	return 1;
}

int RBXScriptSignal::Wait(lua_State *L) {
	RBXScriptSignal *self = LuauStackOp<RBXScriptSignal *>::Check(L, 1);
	return self->emitter->Wait(self->name, L);
}

std::string RBXScriptSignal::ToString() const {
	return "Signal " + name;
}

bool RBXScriptSignal::operator==(const RBXScriptSignal &other) const {
	return emitter == other.emitter && name == other.name;
}

} //namespace SBX::DataTypes

namespace SBX {

using DataTypes::RBXScriptSignal;
UDATA_STACK_OP_IMPL(RBXScriptSignal, "RBXScriptSignal", "RBXScriptSignal", RBXScriptSignalUdata, DTOR(RBXScriptSignal));

} //namespace SBX
