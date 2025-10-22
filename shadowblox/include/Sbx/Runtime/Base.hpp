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

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>

#include "lua.h"

#define luaSBX_propwriteonlyerror(L, propertyName) luaL_error(L, "%s cannot be read", propertyName)
#define luaSBX_nogettererror(L, propertyName, className) luaL_error(L, "%s is not a valid member of %s", propertyName, className)
#define luaSBX_propreadonlyerror(L, propertyName) luaL_error(L, "%s cannot be assigned to", propertyName)
#define luaSBX_nosettererror(L, propertyName) luaSBX_propreadonlyerror(L, propertyName)
#define luaSBX_nonamecallatomerror(L) luaL_error(L, "no namecallatom")
#define luaSBX_aritherror1type(L, op, type) luaL_error(L, "attempt to perform arithmetic (%s) on %s", op, type)
#define luaSBX_aritherror2type(L, op, lhsType, rhsType) luaL_error(L, "attempt to perform arithmetic (%s) on %s and %s", op, lhsType, rhsType)
#define luaSBX_nomethoderror(L, methodName, className) luaSBX_nogettererror(L, methodName, className)

namespace SBX {

enum UdataTag : uint8_t {
	Int64Udata = 0,

	EnumItemUdata,
	EnumUdata,
	EnumsUdata,

	Test1Udata = 124,
	Test2Udata = 125,
	Test3Udata = 126,
	Test4Udata = 127,
};

// https://github.com/Pseudoreality/Roblox-Identities
enum SbxIdentity : uint8_t {
	AnonymousIdentity = 0,
	LocalGuiIdentity,
	GameScriptIdentity,
	ElevatedGameScriptIdentity,
	CommandBarIdentity,
	StudioPluginIdentity,
	ElevatedStudioPluginIdentity,
	COMIdentity,
	WebServiceIdentity,
	ReplicatorIdentity,
	AssistantIdentity,
	OpenCloudSessionIdentity,
	TestingGameScriptIdentity,

	IdentityMax
};

enum SbxCapability {
	// NoneSecurity is ID 0
	// All others are 1 << ID
	NoneSecurity = 0,
	PluginSecurity = 1 << 1,
	LocalUserSecurity = 1 << 3,
	WritePlayerSecurity = 1 << 4,
	RobloxScriptSecurity = 1 << 5,
	RobloxSecurity = 1 << 6,
	NotAccessibleSecurity = 1 << 7,

	AssistantSecurity = 1 << 16,
	InternalTestSecurity = 1 << 17,
	OpenCloudSecurity = 1 << 18,
	RemoteCommandSecurity = 1 << 19,
	UnknownSecurity = 1 << 20
};

enum VMType : uint8_t {
	CoreVM = 0,
	UserVM,

	VMMax
};

struct SbxThreadData {
	VMType vmType = VMMax;
	SbxIdentity identity = AnonymousIdentity;
	int32_t additionalCapability = 0;
	std::shared_ptr<std::mutex> mutex;
	uint64_t interruptDeadline = 0;

	int objRegistry = LUA_NOREF;
	int weakObjRegistry = LUA_NOREF;

	void *userdata = nullptr;
};

lua_State *luaSBX_newstate(VMType vmType, SbxIdentity defaultIdentity);
lua_State *luaSBX_newthread(lua_State *L, SbxIdentity identity);
SbxThreadData *luaSBX_getthreaddata(lua_State *L);
void luaSBX_close(lua_State *L);

int32_t identityToCapabilities(SbxIdentity identity);

bool luaSBX_iscapability(lua_State *L, SbxCapability capability);
void luaSBX_checkcapability(lua_State *L, SbxCapability capability, const char *actionDesc);

bool luaSBX_pushregistry(lua_State *L, void *ptr, void (*push)(lua_State *L, void *ptr), bool weak);

} //namespace SBX
