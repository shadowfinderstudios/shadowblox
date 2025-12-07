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
#include "Sbx/Classes/DataModel.hpp"
#include "Sbx/Classes/Instance.hpp"
#include "Sbx/Classes/Model.hpp"
#include "Sbx/Classes/Part.hpp"
#include "Sbx/Classes/RunService.hpp"
#include "Sbx/Classes/Workspace.hpp"
#include "Sbx/DataTypes/Types.hpp"
#include "Sbx/DataTypes/Vector3.hpp"
#include "Sbx/GodotBridge.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Utils.hpp"

using namespace SBX;
using namespace SBX::Classes;
using namespace SBX::DataTypes;

TEST_SUITE_BEGIN("Classes/RunServiceDataModel");

// Helper to create instances with self-reference set up
static std::shared_ptr<DataModel> MakeDataModel() {
	auto dm = std::make_shared<DataModel>();
	dm->SetSelf(dm);
	return dm;
}

static std::shared_ptr<Workspace> MakeWorkspace() {
	auto ws = std::make_shared<Workspace>();
	ws->SetSelf(ws);
	return ws;
}

static std::shared_ptr<RunService> MakeRunService() {
	auto rs = std::make_shared<RunService>();
	rs->SetSelf(rs);
	return rs;
}

static void InitClasses() {
	static bool initialized = false;
	if (!initialized) {
		Bridge::InitializeAllClasses();
		initialized = true;
	}
}

// ============================================================================
// RunService Tests
// ============================================================================

TEST_CASE("RunService class hierarchy") {
	InitClasses();

	SUBCASE("inheritance") {
		CHECK(ClassDB::IsA("RunService", "Instance"));
		CHECK(ClassDB::IsA("RunService", "Object"));
	}

	SUBCASE("class metadata") {
		const ClassDB::ClassInfo *info = ClassDB::GetClass("RunService");
		REQUIRE_NE(info, nullptr);
		CHECK_EQ(info->name, "RunService");
		CHECK_EQ(info->parent, "Instance");
		CHECK(info->tags.contains(ClassTag::Service));
		CHECK(info->tags.contains(ClassTag::NotCreatable));
	}

	SUBCASE("not creatable via New") {
		auto obj = ClassDB::New("RunService");
		CHECK_EQ(obj, nullptr);
	}
}

TEST_CASE("RunService properties and methods") {
	InitClasses();
	auto rs = MakeRunService();

	SUBCASE("default Name") {
		CHECK_EQ(std::string(rs->GetName()), "RunService");
	}

	SUBCASE("default IsServer") {
		CHECK(rs->IsServer());
	}

	SUBCASE("default IsClient") {
		CHECK_FALSE(rs->IsClient());
	}

	SUBCASE("default IsStudio") {
		CHECK_FALSE(rs->IsStudio());
	}

	SUBCASE("default IsRunning") {
		CHECK_FALSE(rs->IsRunning());
	}

	SUBCASE("Run/Pause/Stop") {
		rs->Run();
		CHECK(rs->IsRunning());
		CHECK(rs->IsRunMode());
		CHECK_FALSE(rs->IsEdit());

		rs->Pause();
		CHECK_FALSE(rs->IsRunning());

		rs->Run();
		rs->Stop();
		CHECK_FALSE(rs->IsRunning());
		CHECK(rs->IsEdit());
	}

	SUBCASE("SetIsClient/SetIsServer") {
		rs->SetIsClient(true);
		rs->SetIsServer(false);
		CHECK(rs->IsClient());
		CHECK_FALSE(rs->IsServer());
	}
}

TEST_CASE("RunService signals") {
	InitClasses();
	auto rs = MakeRunService();

	SUBCASE("signals are registered") {
		const ClassDB::Signal *stepped = ClassDB::GetSignal("RunService", "Stepped");
		REQUIRE_NE(stepped, nullptr);
		CHECK_EQ(stepped->name, "Stepped");
		CHECK_EQ(stepped->parameters.size(), 2);

		const ClassDB::Signal *heartbeat = ClassDB::GetSignal("RunService", "Heartbeat");
		REQUIRE_NE(heartbeat, nullptr);
		CHECK_EQ(heartbeat->name, "Heartbeat");
		CHECK_EQ(heartbeat->parameters.size(), 1);

		const ClassDB::Signal *renderStepped = ClassDB::GetSignal("RunService", "RenderStepped");
		REQUIRE_NE(renderStepped, nullptr);
		CHECK_EQ(renderStepped->name, "RenderStepped");
	}
}

// ============================================================================
// DataModel Tests
// ============================================================================

TEST_CASE("DataModel class hierarchy") {
	InitClasses();

	SUBCASE("inheritance") {
		CHECK(ClassDB::IsA("DataModel", "Instance"));
		CHECK(ClassDB::IsA("DataModel", "Object"));
	}

	SUBCASE("class metadata") {
		const ClassDB::ClassInfo *info = ClassDB::GetClass("DataModel");
		REQUIRE_NE(info, nullptr);
		CHECK_EQ(info->name, "DataModel");
		CHECK_EQ(info->parent, "Instance");
		CHECK(info->tags.contains(ClassTag::NotCreatable));
	}

	SUBCASE("not creatable via New") {
		auto obj = ClassDB::New("DataModel");
		CHECK_EQ(obj, nullptr);
	}
}

TEST_CASE("DataModel properties") {
	InitClasses();
	auto dm = MakeDataModel();

	SUBCASE("default Name") {
		CHECK_EQ(std::string(dm->GetName()), "Game");
	}

	SUBCASE("GameId property") {
		dm->SetGameId("12345");
		CHECK_EQ(std::string(dm->GetGameId()), "12345");
	}

	SUBCASE("PlaceId property") {
		dm->SetPlaceId("67890");
		CHECK_EQ(std::string(dm->GetPlaceId()), "67890");
	}

	SUBCASE("PlaceVersion property") {
		dm->SetPlaceVersion(42);
		CHECK_EQ(dm->GetPlaceVersion(), 42);
	}
}

TEST_CASE("DataModel GetService") {
	InitClasses();
	auto dm = MakeDataModel();

	SUBCASE("GetService creates Workspace") {
		auto ws = dm->GetWorkspace();
		REQUIRE_NE(ws, nullptr);
		CHECK_EQ(std::string(ws->GetClassName()), "Workspace");
		CHECK_EQ(ws->GetParent(), dm);
	}

	SUBCASE("GetService creates RunService") {
		auto rs = dm->GetRunService();
		REQUIRE_NE(rs, nullptr);
		CHECK_EQ(std::string(rs->GetClassName()), "RunService");
		CHECK_EQ(rs->GetParent(), dm);
	}

	SUBCASE("GetService returns same instance") {
		auto ws1 = dm->GetWorkspace();
		auto ws2 = dm->GetWorkspace();
		CHECK_EQ(ws1, ws2);

		auto rs1 = dm->GetRunService();
		auto rs2 = dm->GetRunService();
		CHECK_EQ(rs1, rs2);
	}

	SUBCASE("FindService returns nil for non-existent") {
		auto service = dm->FindService("NonExistent");
		CHECK_EQ(service, nullptr);
	}

	SUBCASE("FindService returns existing service") {
		dm->GetWorkspace();  // Create first
		auto ws = dm->FindService("Workspace");
		REQUIRE_NE(ws, nullptr);
	}
}

// ============================================================================
// Workspace Tests
// ============================================================================

TEST_CASE("Workspace class hierarchy") {
	InitClasses();

	SUBCASE("inheritance") {
		CHECK(ClassDB::IsA("Workspace", "Model"));
		CHECK(ClassDB::IsA("Workspace", "Instance"));
		CHECK(ClassDB::IsA("Workspace", "Object"));
	}

	SUBCASE("class metadata") {
		const ClassDB::ClassInfo *info = ClassDB::GetClass("Workspace");
		REQUIRE_NE(info, nullptr);
		CHECK_EQ(info->name, "Workspace");
		CHECK_EQ(info->parent, "Model");
		CHECK(info->tags.contains(ClassTag::Service));
		CHECK(info->tags.contains(ClassTag::NotCreatable));
	}

	SUBCASE("not creatable via New") {
		auto obj = ClassDB::New("Workspace");
		CHECK_EQ(obj, nullptr);
	}
}

TEST_CASE("Workspace properties") {
	InitClasses();
	auto ws = MakeWorkspace();

	SUBCASE("default Name") {
		CHECK_EQ(std::string(ws->GetName()), "Workspace");
	}

	SUBCASE("default Gravity") {
		Vector3 gravity = ws->GetGravity();
		CHECK_EQ(gravity.X, 0.0);
		CHECK_EQ(gravity.Y, -196.2);
		CHECK_EQ(gravity.Z, 0.0);
	}

	SUBCASE("set Gravity") {
		ws->SetGravity(Vector3(0.0, -9.8, 0.0));
		Vector3 gravity = ws->GetGravity();
		CHECK_EQ(gravity.Y, -9.8);
	}

	SUBCASE("default FallenPartsDestroyHeight") {
		CHECK_EQ(ws->GetFallenPartsDestroyHeight(), -500.0);
	}

	SUBCASE("set FallenPartsDestroyHeight") {
		ws->SetFallenPartsDestroyHeight(-1000.0);
		CHECK_EQ(ws->GetFallenPartsDestroyHeight(), -1000.0);
	}

	SUBCASE("default StreamingEnabled") {
		CHECK_FALSE(ws->GetStreamingEnabled());
	}

	SUBCASE("set StreamingEnabled") {
		ws->SetStreamingEnabled(true);
		CHECK(ws->GetStreamingEnabled());
	}

	SUBCASE("StreamingMinRadius") {
		CHECK_EQ(ws->GetStreamingMinRadius(), 64.0);
		ws->SetStreamingMinRadius(128.0);
		CHECK_EQ(ws->GetStreamingMinRadius(), 128.0);
	}

	SUBCASE("StreamingTargetRadius") {
		CHECK_EQ(ws->GetStreamingTargetRadius(), 1024.0);
		ws->SetStreamingTargetRadius(2048.0);
		CHECK_EQ(ws->GetStreamingTargetRadius(), 2048.0);
	}

	SUBCASE("DistributedGameTime") {
		CHECK_EQ(ws->GetDistributedGameTime(), 0.0);
		ws->UpdateDistributedGameTime(123.456);
		CHECK_EQ(ws->GetDistributedGameTime(), 123.456);
	}
}

// ============================================================================
// Luau API Tests
// ============================================================================

TEST_CASE("DataModel Luau API") {
	InitClasses();

	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	DataTypes::luaSBX_opendatatypes(L);
	ClassDB::Register(L);

	auto dm = MakeDataModel();
	Bridge::RegisterGlobals(L, dm);

	SUBCASE("game global exists") {
		CHECK_EVAL_OK(L, "assert(game ~= nil)");
	}

	SUBCASE("Game global exists (alias)") {
		CHECK_EVAL_OK(L, "assert(Game ~= nil)");
	}

	SUBCASE("game ClassName") {
		CHECK_EVAL_EQ(L, "return game.ClassName", std::string, "DataModel");
	}

	SUBCASE("game:GetService('Workspace')") {
		CHECK_EVAL_OK(L, "local ws = game:GetService('Workspace'); assert(ws ~= nil)");
		CHECK_EVAL_EQ(L, "return game:GetService('Workspace').ClassName", std::string, "Workspace");
	}

	SUBCASE("game:GetService('RunService')") {
		CHECK_EVAL_OK(L, "local rs = game:GetService('RunService'); assert(rs ~= nil)");
		CHECK_EVAL_EQ(L, "return game:GetService('RunService').ClassName", std::string, "RunService");
	}

	SUBCASE("game:FindService returns nil for non-existent") {
		CHECK_EVAL_OK(L, "assert(game:FindService('NonExistent') == nil)");
	}

	SUBCASE("game.Workspace property") {
		CHECK_EVAL_OK(L, "assert(game.Workspace ~= nil)");
		CHECK_EVAL_EQ(L, "return game.Workspace.ClassName", std::string, "Workspace");
	}

	luaSBX_close(L);
}

TEST_CASE("workspace Luau API") {
	InitClasses();

	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	DataTypes::luaSBX_opendatatypes(L);
	ClassDB::Register(L);

	auto dm = MakeDataModel();
	Bridge::RegisterGlobals(L, dm);

	SUBCASE("workspace global exists") {
		CHECK_EVAL_OK(L, "assert(workspace ~= nil)");
	}

	SUBCASE("Workspace global exists (alias)") {
		CHECK_EVAL_OK(L, "assert(Workspace ~= nil)");
	}

	SUBCASE("workspace ClassName") {
		CHECK_EVAL_EQ(L, "return workspace.ClassName", std::string, "Workspace");
	}

	SUBCASE("workspace inherits Model") {
		CHECK_EVAL_EQ(L, "return workspace:IsA('Model')", bool, true);
		CHECK_EVAL_EQ(L, "return workspace:IsA('Instance')", bool, true);
	}

	SUBCASE("workspace Gravity property") {
		EVAL_THEN(L, "return workspace.Gravity.Y", {
			CHECK_EQ(lua_tonumber(L, -1), -196.2);
		});
	}

	SUBCASE("workspace Gravity set") {
		CHECK_EVAL_OK(L, "workspace.Gravity = Vector3.new(0, -9.8, 0)");
		EVAL_THEN(L, "return workspace.Gravity.Y", {
			CHECK_EQ(lua_tonumber(L, -1), -9.8);
		});
	}

	SUBCASE("workspace FallenPartsDestroyHeight") {
		CHECK_EVAL_OK(L, "workspace.FallenPartsDestroyHeight = -1000");
		EVAL_THEN(L, "return workspace.FallenPartsDestroyHeight", {
			CHECK_EQ(lua_tonumber(L, -1), -1000.0);
		});
	}

	SUBCASE("workspace StreamingEnabled") {
		CHECK_EVAL_EQ(L, "return workspace.StreamingEnabled", bool, false);
		CHECK_EVAL_OK(L, "workspace.StreamingEnabled = true");
		CHECK_EVAL_EQ(L, "return workspace.StreamingEnabled", bool, true);
	}

	luaSBX_close(L);
}

TEST_CASE("RunService Luau API") {
	InitClasses();

	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	DataTypes::luaSBX_opendatatypes(L);
	ClassDB::Register(L);

	auto dm = MakeDataModel();
	Bridge::RegisterGlobals(L, dm);

	SUBCASE("RunService accessible via GetService") {
		CHECK_EVAL_OK(L, "local rs = game:GetService('RunService'); assert(rs ~= nil)");
	}

	SUBCASE("RunService ClassName") {
		CHECK_EVAL_EQ(L, "return game:GetService('RunService').ClassName", std::string, "RunService");
	}

	SUBCASE("RunService IsServer method") {
		CHECK_EVAL_EQ(L, "return game:GetService('RunService'):IsServer()", bool, true);
	}

	SUBCASE("RunService IsClient method") {
		CHECK_EVAL_EQ(L, "return game:GetService('RunService'):IsClient()", bool, false);
	}

	SUBCASE("RunService has Heartbeat signal") {
		CHECK_EVAL_OK(L, "local rs = game:GetService('RunService'); assert(rs.Heartbeat ~= nil)");
	}

	SUBCASE("RunService has Stepped signal") {
		CHECK_EVAL_OK(L, "local rs = game:GetService('RunService'); assert(rs.Stepped ~= nil)");
	}

	SUBCASE("RunService has RenderStepped signal") {
		CHECK_EVAL_OK(L, "local rs = game:GetService('RunService'); assert(rs.RenderStepped ~= nil)");
	}

	luaSBX_close(L);
}

TEST_CASE("game/workspace relationship") {
	InitClasses();

	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	DataTypes::luaSBX_opendatatypes(L);
	ClassDB::Register(L);

	auto dm = MakeDataModel();
	Bridge::RegisterGlobals(L, dm);

	SUBCASE("workspace is child of game") {
		CHECK_EVAL_OK(L, "assert(workspace.Parent == game)");
	}

	SUBCASE("workspace is in game:GetChildren()") {
		CHECK_EVAL_OK(L,
			"local found = false "
			"for _, child in ipairs(game:GetChildren()) do "
			"if child == workspace then found = true break end "
			"end "
			"assert(found, 'workspace not found in game:GetChildren()')"
		);
	}

	SUBCASE("workspace == game.Workspace") {
		CHECK_EVAL_OK(L, "assert(workspace == game.Workspace)");
	}

	SUBCASE("workspace == game:GetService('Workspace')") {
		CHECK_EVAL_OK(L, "assert(workspace == game:GetService('Workspace'))");
	}

	luaSBX_close(L);
}

TEST_SUITE_END();
