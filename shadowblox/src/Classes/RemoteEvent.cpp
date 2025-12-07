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

#include "Sbx/Classes/RemoteEvent.hpp"

#include <cstring>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Classes/DataModel.hpp"
#include "Sbx/Classes/Player.hpp"
#include "Sbx/Classes/RunService.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::Classes {

NetworkEventCallback RemoteEvent::networkCallback = nullptr;

RemoteEvent::RemoteEvent() :
		Instance() {
	SetName("RemoteEvent");
}

void RemoteEvent::SetNetworkCallback(NetworkEventCallback callback) {
	networkCallback = callback;
}

NetworkEventCallback RemoteEvent::GetNetworkCallback() {
	return networkCallback;
}

// Serialization format:
// [type:1][data:variable]
// Types: 0=nil, 1=bool, 2=number, 3=string, 4=table(array), 5=Instance
enum class SerialType : uint8_t {
	Nil = 0,
	Bool = 1,
	Number = 2,
	String = 3,
	Table = 4,
	Instance = 5,
	Vector3 = 6,
};

std::vector<uint8_t> RemoteEvent::SerializeArgs(lua_State *L, int argStart, int argCount) {
	std::vector<uint8_t> result;

	// Write argument count
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
				// Write length as 4 bytes
				uint32_t len32 = static_cast<uint32_t>(len);
				const uint8_t *lenBytes = reinterpret_cast<const uint8_t *>(&len32);
				result.insert(result.end(), lenBytes, lenBytes + sizeof(uint32_t));
				// Write string data
				result.insert(result.end(), str, str + len);
				break;
			}
			case LUA_TTABLE: {
				// Serialize as array (simplified - only supports array tables)
				result.push_back(static_cast<uint8_t>(SerialType::Table));
				int tableLen = static_cast<int>(lua_objlen(L, idx));
				result.push_back(static_cast<uint8_t>(tableLen));
				for (int j = 1; j <= tableLen; ++j) {
					lua_rawgeti(L, idx, j);
					auto elemData = SerializeArgs(L, lua_gettop(L), 1);
					// Skip the count byte from nested serialization
					if (!elemData.empty()) {
						result.insert(result.end(), elemData.begin() + 1, elemData.end());
					}
					lua_pop(L, 1);
				}
				break;
			}
			case LUA_TUSERDATA: {
				// Check if it's a Vector3
				if (lua_getmetatable(L, idx)) {
					lua_getfield(L, -1, "__type");
					if (lua_isstring(L, -1)) {
						const char *typeName = lua_tostring(L, -1);
						if (strcmp(typeName, "Vector3") == 0) {
							lua_pop(L, 2); // __type and metatable
							result.push_back(static_cast<uint8_t>(SerialType::Vector3));
							// Get X, Y, Z
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
					lua_pop(L, 2); // __type and metatable
				}
				// Try as Instance
				if (LuauStackOp<std::shared_ptr<Instance>>::Is(L, idx)) {
					result.push_back(static_cast<uint8_t>(SerialType::Instance));
					auto inst = LuauStackOp<std::shared_ptr<Instance>>::Get(L, idx);
					// Serialize instance by full name (simplified approach)
					std::string fullName = inst ? inst->GetFullName() : "";
					uint32_t len = static_cast<uint32_t>(fullName.length());
					const uint8_t *lenBytes = reinterpret_cast<const uint8_t *>(&len);
					result.insert(result.end(), lenBytes, lenBytes + sizeof(uint32_t));
					result.insert(result.end(), fullName.begin(), fullName.end());
				} else {
					// Unknown userdata, serialize as nil
					result.push_back(static_cast<uint8_t>(SerialType::Nil));
				}
				break;
			}
			default: {
				// Unknown type, serialize as nil
				result.push_back(static_cast<uint8_t>(SerialType::Nil));
				break;
			}
		}
	}

	return result;
}

int RemoteEvent::DeserializeArgs(lua_State *L, const std::vector<uint8_t> &data) {
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
			case SerialType::Table: {
				uint8_t tableLen = data[pos++];
				lua_createtable(L, tableLen, 0);
				for (int j = 0; j < tableLen; ++j) {
					// Create a temporary buffer for nested deserialization
					std::vector<uint8_t> tempData;
					tempData.push_back(1); // Single element count
					// Copy the element data
					size_t elemStart = pos;
					auto elemType = static_cast<SerialType>(data[pos++]);
					switch (elemType) {
						case SerialType::Nil:
							break;
						case SerialType::Bool:
							pos++;
							break;
						case SerialType::Number:
							pos += sizeof(double);
							break;
						case SerialType::String:
						case SerialType::Instance: {
							uint32_t len;
							memcpy(&len, &data[pos], sizeof(uint32_t));
							pos += sizeof(uint32_t) + len;
							break;
						}
						case SerialType::Vector3:
							pos += 3 * sizeof(double);
							break;
						default:
							break;
					}
					tempData.insert(tempData.end(), data.begin() + elemStart, data.begin() + pos);
					DeserializeArgs(L, tempData);
					lua_rawseti(L, -2, j + 1);
				}
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
				// Push Vector3.new(x, y, z)
				lua_getglobal(L, "Vector3");
				lua_getfield(L, -1, "new");
				lua_remove(L, -2);
				lua_pushnumber(L, x);
				lua_pushnumber(L, y);
				lua_pushnumber(L, z);
				lua_call(L, 3, 1);
				break;
			}
			case SerialType::Instance: {
				uint32_t len;
				memcpy(&len, &data[pos], sizeof(uint32_t));
				pos += sizeof(uint32_t);
				// For now, push nil for instances (proper resolution would need DataModel access)
				lua_pushnil(L);
				pos += len;
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

void RemoteEvent::FireClient(std::shared_ptr<Player> player, lua_State *L, int argStart, int argCount) {
	if (!player) {
		luaL_error(L, "FireClient: player argument is nil");
		return;
	}

	if (networkCallback) {
		auto data = SerializeArgs(L, argStart, argCount);
		networkCallback(GetName(), player->GetUserId(), data);
	}
}

void RemoteEvent::FireAllClients(lua_State *L, int argStart, int argCount) {
	if (networkCallback) {
		auto data = SerializeArgs(L, argStart, argCount);
		networkCallback(GetName(), -1, data); // -1 means all clients
	}
}

void RemoteEvent::FireServer(lua_State *L, int argStart, int argCount) {
	if (networkCallback) {
		auto data = SerializeArgs(L, argStart, argCount);
		networkCallback(GetName(), 0, data); // 0 means server
	}
}

void RemoteEvent::OnServerEvent(std::shared_ptr<Player> player, lua_State *L, const std::vector<uint8_t> &data) {
	// Push event args and emit signal
	Emit<RemoteEvent>("OnServerEvent", player);
}

void RemoteEvent::OnClientEvent(lua_State *L, const std::vector<uint8_t> &data) {
	// Emit the OnClientEvent signal
	Emit<RemoteEvent>("OnClientEvent");
}

int RemoteEvent::FireClientLuau(lua_State *L) {
	RemoteEvent *self = LuauStackOp<RemoteEvent *>::Check(L, 1);
	std::shared_ptr<Player> player = LuauStackOp<std::shared_ptr<Player>>::Check(L, 2);

	int argCount = lua_gettop(L) - 2; // Subtract self and player
	self->FireClient(player, L, 3, argCount);

	return 0;
}

int RemoteEvent::FireAllClientsLuau(lua_State *L) {
	RemoteEvent *self = LuauStackOp<RemoteEvent *>::Check(L, 1);

	int argCount = lua_gettop(L) - 1; // Subtract self
	self->FireAllClients(L, 2, argCount);

	return 0;
}

int RemoteEvent::FireServerLuau(lua_State *L) {
	RemoteEvent *self = LuauStackOp<RemoteEvent *>::Check(L, 1);

	int argCount = lua_gettop(L) - 1; // Subtract self
	self->FireServer(L, 2, argCount);

	return 0;
}

} // namespace SBX::Classes
