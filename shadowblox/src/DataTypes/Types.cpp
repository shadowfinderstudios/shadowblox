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

#include "Sbx/DataTypes/Types.hpp"

#include "lua.h"

#include "Sbx/DataTypes/Enum.hpp"
#include "Sbx/DataTypes/EnumItem.hpp"
#include "Sbx/DataTypes/EnumTypes.gen.hpp"
#include "Sbx/DataTypes/Enums.hpp"
#include "Sbx/DataTypes/RBXScriptConnection.hpp"
#include "Sbx/DataTypes/RBXScriptSignal.hpp"

namespace SBX::DataTypes {

void luaSBX_opendatatypes(lua_State *L) {
	EnumItem::Register(L);
	Enum::Register(L);
	Enums::Register(L);
	LuauStackOp<Enums *>::Push(L, &enums);
	lua_setglobal(L, "Enum");

	RBXScriptSignal::Register(L);
	RBXScriptConnection::Register(L);
}

} //namespace SBX::DataTypes
