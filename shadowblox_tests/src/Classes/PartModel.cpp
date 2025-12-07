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

#include "doctest.h"

#include <memory>
#include <string>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Classes/ClassDB.hpp"
#include "Sbx/Classes/Instance.hpp"
#include "Sbx/Classes/Model.hpp"
#include "Sbx/Classes/Part.hpp"
#include "Sbx/DataTypes/Types.hpp"
#include "Sbx/DataTypes/Vector3.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Utils.hpp"

using namespace SBX;
using namespace SBX::Classes;
using namespace SBX::DataTypes;

TEST_SUITE_BEGIN("Classes/PartModel");

// Helper to create a Part with self-reference set up
static std::shared_ptr<Part> MakePart() {
	auto part = std::make_shared<Part>();
	part->SetSelf(part);
	return part;
}

// Helper to create a Model with self-reference set up
static std::shared_ptr<Model> MakeModel() {
	auto model = std::make_shared<Model>();
	model->SetSelf(model);
	return model;
}

static void InitClasses() {
	static bool initialized = false;
	if (!initialized) {
		Object::InitializeClass();
		Instance::InitializeClass();
		Part::InitializeClass();
		Model::InitializeClass();
		initialized = true;
	}
}

TEST_CASE("Part class hierarchy") {
	InitClasses();

	SUBCASE("inheritance") {
		CHECK(ClassDB::IsA("Part", "Instance"));
		CHECK(ClassDB::IsA("Part", "Object"));
	}

	SUBCASE("class metadata") {
		const ClassDB::ClassInfo *info = ClassDB::GetClass("Part");
		REQUIRE_NE(info, nullptr);
		CHECK_EQ(info->name, "Part");
		CHECK_EQ(info->parent, "Instance");
	}

	SUBCASE("creatable") {
		auto obj = ClassDB::New("Part");
		CHECK_NE(obj, nullptr);
		CHECK_EQ(std::string(obj->GetClassName()), "Part");
	}
}

TEST_CASE("Part properties") {
	InitClasses();
	auto part = MakePart();

	SUBCASE("default Name") {
		CHECK_EQ(std::string(part->GetName()), "Part");
	}

	SUBCASE("default Size") {
		Vector3 size = part->GetSize();
		CHECK_EQ(size.X, 2.0);
		CHECK_EQ(size.Y, 1.0);
		CHECK_EQ(size.Z, 4.0);
	}

	SUBCASE("set Size") {
		part->SetSize(Vector3(5.0, 5.0, 5.0));
		Vector3 size = part->GetSize();
		CHECK_EQ(size.X, 5.0);
		CHECK_EQ(size.Y, 5.0);
		CHECK_EQ(size.Z, 5.0);
	}

	SUBCASE("Size minimum clamp") {
		part->SetSize(Vector3(0.01, 0.01, 0.01));
		Vector3 size = part->GetSize();
		CHECK_EQ(size.X, 0.05);
		CHECK_EQ(size.Y, 0.05);
		CHECK_EQ(size.Z, 0.05);
	}

	SUBCASE("default Position") {
		Vector3 pos = part->GetPosition();
		CHECK_EQ(pos.X, 0.0);
		CHECK_EQ(pos.Y, 0.0);
		CHECK_EQ(pos.Z, 0.0);
	}

	SUBCASE("set Position") {
		part->SetPosition(Vector3(10.0, 20.0, 30.0));
		Vector3 pos = part->GetPosition();
		CHECK_EQ(pos.X, 10.0);
		CHECK_EQ(pos.Y, 20.0);
		CHECK_EQ(pos.Z, 30.0);
	}

	SUBCASE("default Anchored") {
		CHECK_FALSE(part->GetAnchored());
	}

	SUBCASE("set Anchored") {
		part->SetAnchored(true);
		CHECK(part->GetAnchored());
	}

	SUBCASE("default CanCollide") {
		CHECK(part->GetCanCollide());
	}

	SUBCASE("set CanCollide") {
		part->SetCanCollide(false);
		CHECK_FALSE(part->GetCanCollide());
	}

	SUBCASE("default Transparency") {
		CHECK_EQ(part->GetTransparency(), 0.0);
	}

	SUBCASE("set Transparency") {
		part->SetTransparency(0.5);
		CHECK_EQ(part->GetTransparency(), 0.5);
	}

	SUBCASE("Transparency clamped") {
		part->SetTransparency(1.5);
		CHECK_EQ(part->GetTransparency(), 1.0);
		part->SetTransparency(-0.5);
		CHECK_EQ(part->GetTransparency(), 0.0);
	}

	SUBCASE("default CanTouch") {
		CHECK(part->GetCanTouch());
	}

	SUBCASE("set CanTouch") {
		part->SetCanTouch(false);
		CHECK_FALSE(part->GetCanTouch());
	}
}

TEST_CASE("Model class hierarchy") {
	InitClasses();

	SUBCASE("inheritance") {
		CHECK(ClassDB::IsA("Model", "Instance"));
		CHECK(ClassDB::IsA("Model", "Object"));
	}

	SUBCASE("class metadata") {
		const ClassDB::ClassInfo *info = ClassDB::GetClass("Model");
		REQUIRE_NE(info, nullptr);
		CHECK_EQ(info->name, "Model");
		CHECK_EQ(info->parent, "Instance");
	}

	SUBCASE("creatable") {
		auto obj = ClassDB::New("Model");
		CHECK_NE(obj, nullptr);
		CHECK_EQ(std::string(obj->GetClassName()), "Model");
	}
}

TEST_CASE("Model properties") {
	InitClasses();
	auto model = MakeModel();

	SUBCASE("default Name") {
		CHECK_EQ(std::string(model->GetName()), "Model");
	}

	SUBCASE("default PrimaryPart") {
		CHECK_EQ(model->GetPrimaryPart(), nullptr);
	}

	SUBCASE("set PrimaryPart with descendant") {
		auto part = MakePart();
		part->SetParent(model);

		model->SetPrimaryPart(part);
		CHECK_EQ(model->GetPrimaryPart(), part);
	}

	SUBCASE("PrimaryPart must be descendant") {
		auto part = MakePart();
		// Part is NOT a child of model

		model->SetPrimaryPart(part);
		CHECK_EQ(model->GetPrimaryPart(), nullptr);
	}

	SUBCASE("clear PrimaryPart") {
		auto part = MakePart();
		part->SetParent(model);
		model->SetPrimaryPart(part);

		model->SetPrimaryPart(nullptr);
		CHECK_EQ(model->GetPrimaryPart(), nullptr);
	}
}

TEST_CASE("Model methods") {
	InitClasses();

	SUBCASE("GetExtentsSize empty model") {
		auto model = MakeModel();
		Vector3 size = model->GetExtentsSize();
		CHECK_EQ(size.X, 0.0);
		CHECK_EQ(size.Y, 0.0);
		CHECK_EQ(size.Z, 0.0);
	}

	SUBCASE("GetExtentsSize single part") {
		auto model = MakeModel();
		auto part = MakePart();
		part->SetSize(Vector3(10.0, 10.0, 10.0));
		part->SetPosition(Vector3(0.0, 0.0, 0.0));
		part->SetParent(model);

		Vector3 size = model->GetExtentsSize();
		CHECK_EQ(size.X, 10.0);
		CHECK_EQ(size.Y, 10.0);
		CHECK_EQ(size.Z, 10.0);
	}

	SUBCASE("GetExtentsSize multiple parts") {
		auto model = MakeModel();

		auto part1 = MakePart();
		part1->SetSize(Vector3(2.0, 2.0, 2.0));
		part1->SetPosition(Vector3(0.0, 0.0, 0.0));
		part1->SetParent(model);

		auto part2 = MakePart();
		part2->SetSize(Vector3(2.0, 2.0, 2.0));
		part2->SetPosition(Vector3(10.0, 0.0, 0.0));
		part2->SetParent(model);

		Vector3 size = model->GetExtentsSize();
		// From -1 to 11 on X axis = 12
		CHECK_EQ(size.X, 12.0);
		CHECK_EQ(size.Y, 2.0);
		CHECK_EQ(size.Z, 2.0);
	}

	SUBCASE("TranslateBy") {
		auto model = MakeModel();
		auto part = MakePart();
		part->SetPosition(Vector3(0.0, 0.0, 0.0));
		part->SetParent(model);

		model->TranslateBy(Vector3(5.0, 10.0, 15.0));

		Vector3 pos = part->GetPosition();
		CHECK_EQ(pos.X, 5.0);
		CHECK_EQ(pos.Y, 10.0);
		CHECK_EQ(pos.Z, 15.0);
	}

	SUBCASE("MoveTo") {
		auto model = MakeModel();

		auto primary = MakePart();
		primary->SetPosition(Vector3(0.0, 0.0, 0.0));
		primary->SetParent(model);
		model->SetPrimaryPart(primary);

		auto other = MakePart();
		other->SetPosition(Vector3(5.0, 0.0, 0.0));
		other->SetParent(model);

		model->MoveTo(Vector3(10.0, 10.0, 10.0));

		// Primary should now be at the target
		Vector3 primaryPos = primary->GetPosition();
		CHECK_EQ(primaryPos.X, 10.0);
		CHECK_EQ(primaryPos.Y, 10.0);
		CHECK_EQ(primaryPos.Z, 10.0);

		// Other should have moved by the same offset
		Vector3 otherPos = other->GetPosition();
		CHECK_EQ(otherPos.X, 15.0);
		CHECK_EQ(otherPos.Y, 10.0);
		CHECK_EQ(otherPos.Z, 10.0);
	}
}

TEST_CASE("Part Luau API") {
	InitClasses();

	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	DataTypes::luaSBX_opendatatypes(L);
	ClassDB::Register(L);

	auto part = MakePart();
	LuauStackOp<std::shared_ptr<Part>>::Push(L, part);
	lua_setglobal(L, "part");

	SUBCASE("Size property read") {
		EVAL_THEN(L, "return part.Size.X", {
			CHECK_EQ(lua_tonumber(L, -1), 2.0);
		});
	}

	SUBCASE("Size property write") {
		CHECK_EVAL_OK(L, "part.Size = Vector3.new(8, 8, 8)");
		CHECK_EQ(part->GetSize().X, 8.0);
		CHECK_EQ(part->GetSize().Y, 8.0);
		CHECK_EQ(part->GetSize().Z, 8.0);
	}

	SUBCASE("Position property read") {
		part->SetPosition(Vector3(1.0, 2.0, 3.0));
		EVAL_THEN(L, "return part.Position.X, part.Position.Y, part.Position.Z", {
			CHECK_EQ(lua_tonumber(L, -3), 1.0);
			CHECK_EQ(lua_tonumber(L, -2), 2.0);
			CHECK_EQ(lua_tonumber(L, -1), 3.0);
		});
	}

	SUBCASE("Position property write") {
		CHECK_EVAL_OK(L, "part.Position = Vector3.new(100, 200, 300)");
		CHECK_EQ(part->GetPosition().X, 100.0);
		CHECK_EQ(part->GetPosition().Y, 200.0);
		CHECK_EQ(part->GetPosition().Z, 300.0);
	}

	SUBCASE("Anchored property") {
		CHECK_EVAL_EQ(L, "return part.Anchored", bool, false);
		CHECK_EVAL_OK(L, "part.Anchored = true");
		CHECK_EVAL_EQ(L, "return part.Anchored", bool, true);
	}

	SUBCASE("CanCollide property") {
		CHECK_EVAL_EQ(L, "return part.CanCollide", bool, true);
		CHECK_EVAL_OK(L, "part.CanCollide = false");
		CHECK_EVAL_EQ(L, "return part.CanCollide", bool, false);
	}

	SUBCASE("Transparency property") {
		CHECK_EVAL_OK(L, "part.Transparency = 0.5");
		EVAL_THEN(L, "return part.Transparency", {
			CHECK_EQ(lua_tonumber(L, -1), 0.5);
		});
	}

	SUBCASE("ClassName") {
		CHECK_EVAL_EQ(L, "return part.ClassName", std::string, "Part");
	}

	SUBCASE("IsA") {
		CHECK_EVAL_EQ(L, "return part:IsA('Part')", bool, true);
		CHECK_EVAL_EQ(L, "return part:IsA('Instance')", bool, true);
		CHECK_EVAL_EQ(L, "return part:IsA('Model')", bool, false);
	}

	luaSBX_close(L);
}

TEST_CASE("Model Luau API") {
	InitClasses();

	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	DataTypes::luaSBX_opendatatypes(L);
	ClassDB::Register(L);

	auto model = MakeModel();
	LuauStackOp<std::shared_ptr<Model>>::Push(L, model);
	lua_setglobal(L, "model");

	SUBCASE("ClassName") {
		CHECK_EVAL_EQ(L, "return model.ClassName", std::string, "Model");
	}

	SUBCASE("PrimaryPart default nil") {
		CHECK_EVAL_OK(L, "assert(model.PrimaryPart == nil)");
	}

	SUBCASE("PrimaryPart set and get") {
		auto part = MakePart();
		part->SetParent(model);
		LuauStackOp<std::shared_ptr<Part>>::Push(L, part);
		lua_setglobal(L, "part");

		CHECK_EVAL_OK(L, "model.PrimaryPart = part");
		CHECK_EVAL_OK(L, "assert(model.PrimaryPart == part)");
	}

	SUBCASE("GetExtentsSize") {
		auto part = MakePart();
		part->SetSize(Vector3(4.0, 4.0, 4.0));
		part->SetParent(model);

		EVAL_THEN(L, "return model:GetExtentsSize().X", {
			CHECK_EQ(lua_tonumber(L, -1), 4.0);
		});
	}

	SUBCASE("TranslateBy") {
		auto part = MakePart();
		part->SetPosition(Vector3(0.0, 0.0, 0.0));
		part->SetParent(model);
		LuauStackOp<std::shared_ptr<Part>>::Push(L, part);
		lua_setglobal(L, "part");

		CHECK_EVAL_OK(L, "model:TranslateBy(Vector3.new(5, 5, 5))");
		CHECK_EQ(part->GetPosition().X, 5.0);
		CHECK_EQ(part->GetPosition().Y, 5.0);
		CHECK_EQ(part->GetPosition().Z, 5.0);
	}

	SUBCASE("IsA") {
		CHECK_EVAL_EQ(L, "return model:IsA('Model')", bool, true);
		CHECK_EVAL_EQ(L, "return model:IsA('Instance')", bool, true);
		CHECK_EVAL_EQ(L, "return model:IsA('Part')", bool, false);
	}

	luaSBX_close(L);
}

TEST_SUITE_END();
