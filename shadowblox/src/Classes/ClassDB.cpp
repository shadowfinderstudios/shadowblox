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
#include <string>
#include <unordered_map>
#include <vector>

#include "lua.h"

namespace SBX::Classes {

std::unordered_map<std::string, ClassDB::ClassInfo> ClassDB::classes;
std::unordered_map<std::string, std::shared_ptr<Object> (*)()> ClassDB::constructors;
std::vector<void (*)(lua_State *)> ClassDB::registerCallbacks;

void ClassDB::Register(lua_State *L) {
	for (auto cb : registerCallbacks) {
		cb(L);
	}
}

const ClassDB::ClassInfo *ClassDB::GetClass(const std::string &className) {
	auto c = classes.find(className);
	if (c == classes.end()) {
		return nullptr;
	}

	return &c->second;
}

const ClassDB::Function *ClassDB::GetFunction(const std::string &className, const std::string &funcName) {
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

const ClassDB::Property *ClassDB::GetProperty(const std::string &className, const std::string &propName) {
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

const ClassDB::Signal *ClassDB::GetSignal(const std::string &className, const std::string &sigName) {
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

const ClassDB::Callback *ClassDB::GetCallback(const std::string &className, const std::string &cbName) {
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

std::shared_ptr<Object> ClassDB::New(const std::string &className) {
	auto ctor = constructors.find(className);
	return ctor != constructors.end() ? ctor->second() : nullptr;
}

bool ClassDB::IsA(const std::string &derived, const std::string &base) {
	if (!classes.contains(base)) {
		return false;
	}

	const std::string *curr = &derived;
	while (!curr->empty()) {
		if (*curr == base) {
			return true;
		}

		curr = &classes[*curr].parent;
	}

	return false;
}

} //namespace SBX::Classes
