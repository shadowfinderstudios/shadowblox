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
#include <vector>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Classes/ClassDB.hpp"
#include "Sbx/Classes/Instance.hpp"
#include "Sbx/DataTypes/Types.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/SignalEmitter.hpp"
#include "Utils.hpp"

using namespace SBX;
using namespace SBX::Classes;

TEST_SUITE_BEGIN("Classes/Instance");

namespace SBX::Classes {

// Test class that inherits from Instance (since Instance is NotCreatable)
class TestInstance : public Instance {
	SBXCLASS(TestInstance, Instance, MemoryCategory::Internal, ClassTag::NotReplicated);

public:
	TestInstance() = default;
	~TestInstance() override = default;

protected:
	template <typename T>
	static void BindMembers() {
		Instance::BindMembers<T>();
	}
};

} //namespace SBX::Classes

// Helper to create a TestInstance with self-reference set up
static std::shared_ptr<TestInstance> MakeTestInstance() {
	auto inst = std::make_shared<TestInstance>();
	inst->SetSelf(inst);
	return inst;
}

TEST_CASE("class hierarchy") {
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();

	SUBCASE("inheritance") {
		CHECK(ClassDB::IsA("TestInstance", "Instance"));
		CHECK(ClassDB::IsA("TestInstance", "Object"));
		CHECK(ClassDB::IsA("Instance", "Object"));
	}

	SUBCASE("class metadata") {
		const ClassDB::ClassInfo *info = ClassDB::GetClass("Instance");
		REQUIRE_NE(info, nullptr);
		CHECK_EQ(info->name, "Instance");
		CHECK_EQ(info->parent, "Object");
	}
}

TEST_CASE("properties") {
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();

	auto inst = MakeTestInstance();

	SUBCASE("Name default") {
		CHECK_EQ(std::string(inst->GetName()), "Instance");
	}

	SUBCASE("Name set") {
		inst->SetName("TestPart");
		CHECK_EQ(std::string(inst->GetName()), "TestPart");
	}

	SUBCASE("Parent default") {
		CHECK_EQ(inst->GetParent(), nullptr);
	}
}

TEST_CASE("parent-child relationships") {
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();

	SUBCASE("set parent") {
		auto parent = MakeTestInstance();
		auto child = MakeTestInstance();

		child->SetParent(parent);

		CHECK_EQ(child->GetParent(), parent);
		CHECK_EQ(parent->GetChildren().size(), 1);
		CHECK_EQ(parent->GetChildren()[0], child);
	}

	SUBCASE("change parent") {
		auto parent1 = MakeTestInstance();
		auto parent2 = MakeTestInstance();
		auto child = MakeTestInstance();

		child->SetParent(parent1);
		CHECK_EQ(parent1->GetChildren().size(), 1);
		CHECK_EQ(parent2->GetChildren().size(), 0);

		child->SetParent(parent2);
		CHECK_EQ(parent1->GetChildren().size(), 0);
		CHECK_EQ(parent2->GetChildren().size(), 1);
		CHECK_EQ(child->GetParent(), parent2);
	}

	SUBCASE("remove parent (set nil)") {
		auto parent = MakeTestInstance();
		auto child = MakeTestInstance();

		child->SetParent(parent);
		CHECK_EQ(parent->GetChildren().size(), 1);

		child->SetParent(nullptr);
		CHECK_EQ(child->GetParent(), nullptr);
		CHECK_EQ(parent->GetChildren().size(), 0);
	}

	SUBCASE("prevent circular parenting") {
		auto grandparent = MakeTestInstance();
		auto parent = MakeTestInstance();
		auto child = MakeTestInstance();

		parent->SetParent(grandparent);
		child->SetParent(parent);

		// Trying to set grandparent's parent to child should fail
		grandparent->SetParent(child);
		CHECK_EQ(grandparent->GetParent(), nullptr);
	}

	SUBCASE("multiple children") {
		auto parent = MakeTestInstance();
		auto child1 = MakeTestInstance();
		auto child2 = MakeTestInstance();
		auto child3 = MakeTestInstance();

		child1->SetName("Child1");
		child2->SetName("Child2");
		child3->SetName("Child3");

		child1->SetParent(parent);
		child2->SetParent(parent);
		child3->SetParent(parent);

		CHECK_EQ(parent->GetChildren().size(), 3);
	}
}

TEST_CASE("GetDescendants") {
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();

	auto root = MakeTestInstance();
	auto child1 = MakeTestInstance();
	auto child2 = MakeTestInstance();
	auto grandchild1 = MakeTestInstance();
	auto grandchild2 = MakeTestInstance();

	child1->SetParent(root);
	child2->SetParent(root);
	grandchild1->SetParent(child1);
	grandchild2->SetParent(child1);

	auto descendants = root->GetDescendants();
	CHECK_EQ(descendants.size(), 4);
}

TEST_CASE("FindFirstChild") {
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();

	auto parent = MakeTestInstance();
	auto child1 = MakeTestInstance();
	auto child2 = MakeTestInstance();
	auto grandchild = MakeTestInstance();

	child1->SetName("Child1");
	child2->SetName("Child2");
	grandchild->SetName("Grandchild");

	child1->SetParent(parent);
	child2->SetParent(parent);
	grandchild->SetParent(child1);

	SUBCASE("find direct child") {
		auto found = parent->FindFirstChild("Child1");
		CHECK_EQ(found, child1);
	}

	SUBCASE("not find grandchild without recursive") {
		auto found = parent->FindFirstChild("Grandchild", false);
		CHECK_EQ(found, nullptr);
	}

	SUBCASE("find grandchild with recursive") {
		auto found = parent->FindFirstChild("Grandchild", true);
		CHECK_EQ(found, grandchild);
	}

	SUBCASE("not found") {
		auto found = parent->FindFirstChild("NonExistent");
		CHECK_EQ(found, nullptr);
	}
}

TEST_CASE("FindFirstChildOfClass") {
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();

	auto parent = MakeTestInstance();
	auto child = MakeTestInstance();

	child->SetParent(parent);

	auto found = parent->FindFirstChildOfClass("TestInstance");
	CHECK_EQ(found, child);

	auto notFound = parent->FindFirstChildOfClass("NonExistent");
	CHECK_EQ(notFound, nullptr);
}

TEST_CASE("FindFirstAncestor") {
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();

	auto root = MakeTestInstance();
	auto middle = MakeTestInstance();
	auto leaf = MakeTestInstance();

	root->SetName("Root");
	middle->SetName("Middle");
	leaf->SetName("Leaf");

	middle->SetParent(root);
	leaf->SetParent(middle);

	SUBCASE("find immediate parent") {
		auto found = leaf->FindFirstAncestor("Middle");
		CHECK_EQ(found, middle);
	}

	SUBCASE("find grandparent") {
		auto found = leaf->FindFirstAncestor("Root");
		CHECK_EQ(found, root);
	}

	SUBCASE("not found") {
		auto found = leaf->FindFirstAncestor("NonExistent");
		CHECK_EQ(found, nullptr);
	}
}

TEST_CASE("FindFirstChildWhichIsA") {
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();

	auto parent = MakeTestInstance();
	auto child = MakeTestInstance();

	child->SetParent(parent);

	SUBCASE("find by exact class") {
		auto found = parent->FindFirstChildWhichIsA("TestInstance");
		CHECK_EQ(found, child);
	}

	SUBCASE("find by base class") {
		auto found = parent->FindFirstChildWhichIsA("Instance");
		CHECK_EQ(found, child);
	}

	SUBCASE("find by Object") {
		auto found = parent->FindFirstChildWhichIsA("Object");
		CHECK_EQ(found, child);
	}
}

TEST_CASE("IsAncestorOf and IsDescendantOf") {
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();

	auto root = MakeTestInstance();
	auto middle = MakeTestInstance();
	auto leaf = MakeTestInstance();
	auto unrelated = MakeTestInstance();

	middle->SetParent(root);
	leaf->SetParent(middle);

	SUBCASE("IsAncestorOf") {
		CHECK(root->IsAncestorOf(middle.get()));
		CHECK(root->IsAncestorOf(leaf.get()));
		CHECK(middle->IsAncestorOf(leaf.get()));
		CHECK_FALSE(leaf->IsAncestorOf(root.get()));
		CHECK_FALSE(root->IsAncestorOf(unrelated.get()));
	}

	SUBCASE("IsDescendantOf") {
		CHECK(leaf->IsDescendantOf(middle.get()));
		CHECK(leaf->IsDescendantOf(root.get()));
		CHECK(middle->IsDescendantOf(root.get()));
		CHECK_FALSE(root->IsDescendantOf(leaf.get()));
		CHECK_FALSE(unrelated->IsDescendantOf(root.get()));
	}
}

TEST_CASE("GetFullName") {
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();

	auto root = MakeTestInstance();
	auto middle = MakeTestInstance();
	auto leaf = MakeTestInstance();

	root->SetName("Root");
	middle->SetName("Middle");
	leaf->SetName("Leaf");

	middle->SetParent(root);
	leaf->SetParent(middle);

	CHECK_EQ(root->GetFullName(), "Root");
	CHECK_EQ(middle->GetFullName(), "Root.Middle");
	CHECK_EQ(leaf->GetFullName(), "Root.Middle.Leaf");
}

TEST_CASE("Destroy") {
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();

	SUBCASE("removes from parent") {
		auto parent = MakeTestInstance();
		auto child = MakeTestInstance();

		child->SetParent(parent);
		CHECK_EQ(parent->GetChildren().size(), 1);

		child->Destroy();
		CHECK_EQ(parent->GetChildren().size(), 0);
		CHECK(child->IsDestroyed());
	}

	SUBCASE("destroys children") {
		auto parent = MakeTestInstance();
		auto child1 = MakeTestInstance();
		auto child2 = MakeTestInstance();

		child1->SetParent(parent);
		child2->SetParent(parent);

		parent->Destroy();
		CHECK(parent->IsDestroyed());
		CHECK(child1->IsDestroyed());
		CHECK(child2->IsDestroyed());
	}
}

TEST_CASE("ClearAllChildren") {
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();

	auto parent = MakeTestInstance();
	auto child1 = MakeTestInstance();
	auto child2 = MakeTestInstance();
	auto grandchild = MakeTestInstance();

	child1->SetParent(parent);
	child2->SetParent(parent);
	grandchild->SetParent(child1);

	parent->ClearAllChildren();

	CHECK_EQ(parent->GetChildren().size(), 0);
	CHECK(child1->IsDestroyed());
	CHECK(child2->IsDestroyed());
	CHECK(grandchild->IsDestroyed());
	CHECK_FALSE(parent->IsDestroyed());
}

TEST_CASE("stack operations") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();
	ClassDB::Register(L);

	using Ref = std::shared_ptr<TestInstance>;

	auto inst = MakeTestInstance();

	SUBCASE("push and get") {
		LuauStackOp<Ref>::Push(L, inst);
		CHECK(LuauStackOp<Ref>::Is(L, -1));

		Ref got = LuauStackOp<Ref>::Get(L, -1);
		CHECK_EQ(got, inst);
	}

	SUBCASE("check Instance type") {
		LuauStackOp<Ref>::Push(L, inst);
		CHECK(LuauStackOp<std::shared_ptr<Instance>>::Is(L, -1));
	}

	luaSBX_close(L);
}

TEST_CASE("Luau API") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	DataTypes::luaSBX_opendatatypes(L);
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();
	ClassDB::Register(L);

	SignalConnectionOwner connections;
	SbxThreadData *udata = luaSBX_getthreaddata(L);
	udata->signalConnections = &connections;

	auto parent = MakeTestInstance();
	parent->SetName("Parent");
	auto child = MakeTestInstance();
	child->SetName("Child");
	child->SetParent(parent);

	LuauStackOp<std::shared_ptr<TestInstance>>::Push(L, parent);
	lua_setglobal(L, "parent");
	LuauStackOp<std::shared_ptr<TestInstance>>::Push(L, child);
	lua_setglobal(L, "child");

	SUBCASE("Name property") {
		CHECK_EVAL_EQ(L, "return parent.Name", std::string, "Parent");
		CHECK_EVAL_OK(L, "parent.Name = 'NewName'");
		CHECK_EQ(std::string(parent->GetName()), "NewName");
	}

	SUBCASE("Parent property read") {
		CHECK_EVAL_OK(L, "assert(child.Parent == parent)");
	}

	SUBCASE("Parent property write") {
		auto newParent = MakeTestInstance();
		newParent->SetName("NewParent");
		LuauStackOp<std::shared_ptr<TestInstance>>::Push(L, newParent);
		lua_setglobal(L, "newParent");

		CHECK_EVAL_OK(L, "child.Parent = newParent");
		CHECK_EQ(child->GetParent(), newParent);
	}

	SUBCASE("Parent property nil") {
		CHECK_EVAL_OK(L, "child.Parent = nil");
		CHECK_EQ(child->GetParent(), nullptr);
	}

	SUBCASE("GetChildren") {
		CHECK_EVAL_OK(L, "local children = parent:GetChildren(); assert(#children == 1)");
	}

	SUBCASE("FindFirstChild") {
		CHECK_EVAL_OK(L, "assert(parent:FindFirstChild('Child') == child)");
		CHECK_EVAL_OK(L, "assert(parent:FindFirstChild('NonExistent') == nil)");
	}

	SUBCASE("GetFullName") {
		CHECK_EVAL_EQ(L, "return child:GetFullName()", std::string, "Parent.Child");
	}

	SUBCASE("IsAncestorOf") {
		CHECK_EVAL_EQ(L, "return parent:IsAncestorOf(child)", bool, true);
		CHECK_EVAL_EQ(L, "return child:IsAncestorOf(parent)", bool, false);
	}

	SUBCASE("IsDescendantOf") {
		CHECK_EVAL_EQ(L, "return child:IsDescendantOf(parent)", bool, true);
		CHECK_EVAL_EQ(L, "return parent:IsDescendantOf(child)", bool, false);
	}

	SUBCASE("ClassName") {
		CHECK_EVAL_EQ(L, "return parent.ClassName", std::string, "TestInstance");
	}

	SUBCASE("IsA") {
		CHECK_EVAL_EQ(L, "return parent:IsA('TestInstance')", bool, true);
		CHECK_EVAL_EQ(L, "return parent:IsA('Instance')", bool, true);
		CHECK_EVAL_EQ(L, "return parent:IsA('Object')", bool, true);
	}

	luaSBX_close(L);
}

TEST_CASE("signals") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	DataTypes::luaSBX_opendatatypes(L);
	Object::InitializeClass();
	Instance::InitializeClass();
	TestInstance::InitializeClass();
	ClassDB::Register(L);

	SignalConnectionOwner connections;
	SbxThreadData *udata = luaSBX_getthreaddata(L);
	udata->signalConnections = &connections;

	auto parent = MakeTestInstance();
	auto child = MakeTestInstance();

	LuauStackOp<std::shared_ptr<TestInstance>>::Push(L, parent);
	lua_setglobal(L, "parent");
	LuauStackOp<std::shared_ptr<TestInstance>>::Push(L, child);
	lua_setglobal(L, "child");

	SUBCASE("ChildAdded signal") {
		CHECK_EVAL_OK(L, R"(
			addedChild = nil
			parent.ChildAdded:Connect(function(c)
				addedChild = c
			end)
		)");

		child->SetParent(parent);

		lua_getglobal(L, "addedChild");
		CHECK_FALSE(lua_isnil(L, -1));
		auto got = LuauStackOp<std::shared_ptr<Instance>>::Get(L, -1);
		CHECK_EQ(got, child);
		lua_pop(L, 1);
	}

	SUBCASE("ChildRemoved signal") {
		child->SetParent(parent);

		CHECK_EVAL_OK(L, R"(
			removedChild = nil
			parent.ChildRemoved:Connect(function(c)
				removedChild = c
			end)
		)");

		child->SetParent(nullptr);

		lua_getglobal(L, "removedChild");
		CHECK_FALSE(lua_isnil(L, -1));
		auto got = LuauStackOp<std::shared_ptr<Instance>>::Get(L, -1);
		CHECK_EQ(got, child);
		lua_pop(L, 1);
	}

	luaSBX_close(L);
}

TEST_SUITE_END();
