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

#include "Sbx/Classes/RemoteFunction.hpp"

#include <cstring>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Classes/Player.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::Classes {

NetworkFunctionCallback RemoteFunction::networkCallback = nullptr;

RemoteFunction::RemoteFunction() :
		Instance() {
	SetName("RemoteFunction");
}

void RemoteFunction::SetNetworkCallback(NetworkFunctionCallback callback) {
	networkCallback = callback;
}

NetworkFunctionCallback RemoteFunction::GetNetworkCallback() {
	return networkCallback;
}

// Reuse the same serialization format as RemoteEvent
enum class SerialType : uint8_t {
	Nil = 0,
	Bool = 1,
	Number = 2,
	String = 3,
	Table = 4,
	Instance = 5,
	Vector3 = 6,
};

std::vector<uint8_t> RemoteFunction::SerializeArgs(lua_State *L, int argStart, int argCount) {
	std::vector<uint8_t> result;
	result.push_back(static_cast<uint8_t>(argCount));

	for (int i = 0; i < argCount; ++i) {
		int idx = argStart + i;
		int type = lua_type(L, idx);

		switch (type) {
			case LUA_TNIL: {
				result.push_back(static_cast<uint8_t>(SerialType::Nil));
				break;
			}
			case LUA_TBOOLEAN: {
				result.push_back(static_cast<uint8_t>(SerialType::Bool));
				result.push_back(lua_toboolean(L, idx) ? 1 : 0);
				break;
			}
			case LUA_TNUMBER: {
				result.push_back(static_cast<uint8_t>(SerialType::Number));
				double num = lua_tonumber(L, idx);
				const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&num);
				result.insert(result.end(), bytes, bytes + sizeof(double));
				break;
			}
			case LUA_TSTRING: {
				result.push_back(static_cast<uint8_t>(SerialType::String));
				size_t len;
				const char *str = lua_tolstring(L, idx, &len);
				uint32_t len32 = static_cast<uint32_t>(len);
				const uint8_t *lenBytes = reinterpret_cast<const uint8_t *>(&len32);
				result.insert(result.end(), lenBytes, lenBytes + sizeof(uint32_t));
				result.insert(result.end(), str, str + len);
				break;
			}
			case LUA_TUSERDATA: {
				// Check for Vector3 or Instance
				if (lua_getmetatable(L, idx)) {
					lua_getfield(L, -1, "__type");
					if (lua_isstring(L, -1)) {
						const char *typeName = lua_tostring(L, -1);
						if (strcmp(typeName, "Vector3") == 0) {
							lua_pop(L, 2);
							result.push_back(static_cast<uint8_t>(SerialType::Vector3));
							lua_getfield(L, idx, "X");
							lua_getfield(L, idx, "Y");
							lua_getfield(L, idx, "Z");
							double x = lua_tonumber(L, -3);
							double y = lua_tonumber(L, -2);
							double z = lua_tonumber(L, -1);
							lua_pop(L, 3);
							const uint8_t *xBytes = reinterpret_cast<const uint8_t *>(&x);
							const uint8_t *yBytes = reinterpret_cast<const uint8_t *>(&y);
							const uint8_t *zBytes = reinterpret_cast<const uint8_t *>(&z);
							result.insert(result.end(), xBytes, xBytes + sizeof(double));
							result.insert(result.end(), yBytes, yBytes + sizeof(double));
							result.insert(result.end(), zBytes, zBytes + sizeof(double));
							break;
						}
					}
					lua_pop(L, 2);
				}
				// Default to nil for unknown userdata
				result.push_back(static_cast<uint8_t>(SerialType::Nil));
				break;
			}
			default: {
				result.push_back(static_cast<uint8_t>(SerialType::Nil));
				break;
			}
		}
	}

	return result;
}

int RemoteFunction::DeserializeArgs(lua_State *L, const std::vector<uint8_t> &data) {
	if (data.empty()) {
		return 0;
	}

	size_t pos = 0;
	uint8_t argCount = data[pos++];

	for (int i = 0; i < argCount && pos < data.size(); ++i) {
		auto type = static_cast<SerialType>(data[pos++]);

		switch (type) {
			case SerialType::Nil: {
				lua_pushnil(L);
				break;
			}
			case SerialType::Bool: {
				lua_pushboolean(L, data[pos++] != 0);
				break;
			}
			case SerialType::Number: {
				double num;
				memcpy(&num, &data[pos], sizeof(double));
				pos += sizeof(double);
				lua_pushnumber(L, num);
				break;
			}
			case SerialType::String: {
				uint32_t len;
				memcpy(&len, &data[pos], sizeof(uint32_t));
				pos += sizeof(uint32_t);
				lua_pushlstring(L, reinterpret_cast<const char *>(&data[pos]), len);
				pos += len;
				break;
			}
			case SerialType::Vector3: {
				double x, y, z;
				memcpy(&x, &data[pos], sizeof(double));
				pos += sizeof(double);
				memcpy(&y, &data[pos], sizeof(double));
				pos += sizeof(double);
				memcpy(&z, &data[pos], sizeof(double));
				pos += sizeof(double);
				lua_getglobal(L, "Vector3");
				lua_getfield(L, -1, "new");
				lua_remove(L, -2);
				lua_pushnumber(L, x);
				lua_pushnumber(L, y);
				lua_pushnumber(L, z);
				lua_call(L, 3, 1);
				break;
			}
			default: {
				lua_pushnil(L);
				break;
			}
		}
	}

	return argCount;
}

int RemoteFunction::InvokeServer(lua_State *L) {
	int argCount = lua_gettop(L) - 1;

	if (networkCallback) {
		auto data = SerializeArgs(L, 2, argCount);
		auto response = networkCallback(GetName(), 0, data);
		return DeserializeArgs(L, response);
	}

	return 0;
}

int RemoteFunction::InvokeClient(std::shared_ptr<Player> player, lua_State *L) {
	if (!player) {
		luaL_error(L, "InvokeClient: player argument is nil");
		return 0;
	}

	int argCount = lua_gettop(L) - 2;

	if (networkCallback) {
		auto data = SerializeArgs(L, 3, argCount);
		auto response = networkCallback(GetName(), player->GetUserId(), data);
		return DeserializeArgs(L, response);
	}

	return 0;
}

void RemoteFunction::SetOnServerInvoke(lua_State *L) {
	if (onServerInvokeRef != LUA_NOREF && serverInvokeState) {
		lua_unref(serverInvokeState, onServerInvokeRef);
	}

	if (lua_isfunction(L, -1)) {
		onServerInvokeRef = lua_ref(L, -1);
		serverInvokeState = L;
	} else if (lua_isnil(L, -1)) {
		onServerInvokeRef = LUA_NOREF;
		serverInvokeState = nullptr;
	} else {
		luaL_typeerror(L, -1, "function");
	}
}

void RemoteFunction::SetOnClientInvoke(lua_State *L) {
	if (onClientInvokeRef != LUA_NOREF && clientInvokeState) {
		lua_unref(clientInvokeState, onClientInvokeRef);
	}

	if (lua_isfunction(L, -1)) {
		onClientInvokeRef = lua_ref(L, -1);
		clientInvokeState = L;
	} else if (lua_isnil(L, -1)) {
		onClientInvokeRef = LUA_NOREF;
		clientInvokeState = nullptr;
	} else {
		luaL_typeerror(L, -1, "function");
	}
}

std::vector<uint8_t> RemoteFunction::HandleServerInvoke(std::shared_ptr<Player> player, lua_State *L, const std::vector<uint8_t> &data) {
	if (onServerInvokeRef == LUA_NOREF || !serverInvokeState) {
		return {};
	}

	lua_getref(serverInvokeState, onServerInvokeRef);
	LuauStackOp<std::shared_ptr<Instance>>::Push(serverInvokeState, player);
	int argCount = DeserializeArgs(serverInvokeState, data);

	if (lua_pcall(serverInvokeState, 1 + argCount, LUA_MULTRET, 0) != 0) {
		lua_pop(serverInvokeState, 1); // error message
		return {};
	}

	int nresults = lua_gettop(serverInvokeState);
	auto result = SerializeArgs(serverInvokeState, 1, nresults);
	lua_pop(serverInvokeState, nresults);
	return result;
}

std::vector<uint8_t> RemoteFunction::HandleClientInvoke(lua_State *L, const std::vector<uint8_t> &data) {
	if (onClientInvokeRef == LUA_NOREF || !clientInvokeState) {
		return {};
	}

	lua_getref(clientInvokeState, onClientInvokeRef);
	int argCount = DeserializeArgs(clientInvokeState, data);

	if (lua_pcall(clientInvokeState, argCount, LUA_MULTRET, 0) != 0) {
		lua_pop(clientInvokeState, 1);
		return {};
	}

	int nresults = lua_gettop(clientInvokeState);
	auto result = SerializeArgs(clientInvokeState, 1, nresults);
	lua_pop(clientInvokeState, nresults);
	return result;
}

int RemoteFunction::InvokeServerLuau(lua_State *L) {
	RemoteFunction *self = LuauStackOp<RemoteFunction *>::Check(L, 1);
	return self->InvokeServer(L);
}

int RemoteFunction::InvokeClientLuau(lua_State *L) {
	RemoteFunction *self = LuauStackOp<RemoteFunction *>::Check(L, 1);
	std::shared_ptr<Player> player = LuauStackOp<std::shared_ptr<Player>>::Check(L, 2);
	return self->InvokeClient(player, L);
}

} // namespace SBX::Classes
