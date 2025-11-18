// SPDX-License-Identifier: LGPL-3.0-or-later
/*******************************************************************************
 * shadowblox - https://git.seki.pw/Fumohouse/shadowblox
 *
 * Copyright 2025-present ksk.
 * Copyright 2025-present shadowblox contributors.
 *
 * Licensed under the GNU Lesser General Public License version 3.0 or later.
 * See COPYRIGHT.txt for more details.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "Sbx/Classes/Object.hpp"

#include <memory>

#include "lua.h"

#include "Sbx/Classes/ClassDB.hpp"
#include "Sbx/DataTypes/RBXScriptSignal.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/SignalEmitter.hpp"

namespace SBX::Classes {

Object::Object() :
		emitter(std::make_shared<SignalEmitter>()) {
}

Object::~Object() {}

bool Object::IsA(const char *className) const {
	return ClassDB::IsA(GetClassName(), className);
}

void Object::PushSignal(lua_State *L, const std::string &name, SbxCapability security) {
	LuauStackOp<DataTypes::RBXScriptSignal>::Push(L, DataTypes::RBXScriptSignal(emitter, name, security));
}

} //namespace SBX::Classes
