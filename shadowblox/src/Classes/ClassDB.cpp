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

#include "Sbx/Classes/ClassDB.hpp"

#include <memory>
#include <string_view>
#include <vector>

#include "lua.h"

#include "Sbx/Runtime/StringMap.hpp"

namespace SBX::Classes {

StringMap<ClassDB::ClassInfo> ClassDB::classes;
StringMap<std::shared_ptr<Object> (*)()> ClassDB::constructors;
std::vector<void (*)(lua_State *)> ClassDB::registerCallbacks;

void ClassDB::Register(lua_State *L) {
	for (auto cb : registerCallbacks) {
		cb(L);
	}
}

const ClassDB::ClassInfo *ClassDB::GetClass(std::string_view className) {
	auto c = classes.find(className);
	if (c == classes.end()) {
		return nullptr;
	}

	return &c->second;
}

const ClassDB::Function *ClassDB::GetFunction(std::string_view className, std::string_view funcName) {
	auto c = classes.find(className);
	if (c == classes.end()) {
		return nullptr;
	}

	auto f = c->second.functions.find(funcName);
	if (f == c->second.functions.end()) {
		return nullptr;
	}

	return &f->second;
}

const ClassDB::Property *ClassDB::GetProperty(std::string_view className, std::string_view propName) {
	auto c = classes.find(className);
	if (c == classes.end()) {
		return nullptr;
	}

	auto p = c->second.properties.find(propName);
	if (p == c->second.properties.end()) {
		return nullptr;
	}

	return &p->second;
}

const ClassDB::Signal *ClassDB::GetSignal(std::string_view className, std::string_view sigName) {
	auto c = classes.find(className);
	if (c == classes.end()) {
		return nullptr;
	}

	auto s = c->second.signals.find(sigName);
	if (s == c->second.signals.end()) {
		return nullptr;
	}

	return &s->second;
}

const ClassDB::Callback *ClassDB::GetCallback(std::string_view className, std::string_view cbName) {
	auto c = classes.find(className);
	if (c == classes.end()) {
		return nullptr;
	}

	auto cb = c->second.callbacks.find(cbName);
	if (cb == c->second.callbacks.end()) {
		return nullptr;
	}

	return &cb->second;
}

std::shared_ptr<Object> ClassDB::New(std::string_view className) {
	auto ctor = constructors.find(className);
	return ctor != constructors.end() ? ctor->second() : nullptr;
}

bool ClassDB::IsA(std::string_view derived, std::string_view base) {
	if (!classes.contains(base)) {
		return false;
	}

	std::string_view curr = derived;
	while (!curr.empty()) {
		if (curr == base) {
			return true;
		}

		curr = classes.find(curr)->second.parent;
	}

	return false;
}

} //namespace SBX::Classes
