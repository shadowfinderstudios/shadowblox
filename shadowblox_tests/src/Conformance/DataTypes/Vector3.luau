-- SPDX-License-Identifier: MPL-2.0
--[============================================================================[
-- shadowblox - https://git.seki.pw/Fumohouse/shadowblox
--
-- Copyright 2025-present ksk.
-- Copyright 2025-present shadowblox contributors.
--
-- This Source Code Form is subject to the terms of the Mozilla Public License,
-- v. 2.0. If a copy of the MPL was not distributed with this file, You can
-- obtain one at https://mozilla.org/MPL/2.0/.
--
-- See COPYRIGHT.txt for more details.
--]============================================================================]

-- Constructors
do
	local v1 = Vector3.new()
	assert(v1.X == 0)
	assert(v1.Y == 0)
	assert(v1.Z == 0)

	local v2 = Vector3.new(1, 2, 3)
	assert(v2.X == 1)
	assert(v2.Y == 2)
	assert(v2.Z == 3)

	local v3 = Vector3.new(5)
	assert(v3.X == 5)
	assert(v3.Y == 5)
	assert(v3.Z == 5)

	local v4 = Vector3.new(1, 2)
	assert(v4.X == 1)
	assert(v4.Y == 2)
	assert(v4.Z == 0)
end

-- Constants
do
	assert(Vector3.zero.X == 0 and Vector3.zero.Y == 0 and Vector3.zero.Z == 0)
	assert(Vector3.one.X == 1 and Vector3.one.Y == 1 and Vector3.one.Z == 1)
	assert(Vector3.xAxis.X == 1 and Vector3.xAxis.Y == 0 and Vector3.xAxis.Z == 0)
	assert(Vector3.yAxis.X == 0 and Vector3.yAxis.Y == 1 and Vector3.yAxis.Z == 0)
	assert(Vector3.zAxis.X == 0 and Vector3.zAxis.Y == 0 and Vector3.zAxis.Z == 1)
end

-- Properties
do
	local v = Vector3.new(3, 4, 0)
	assert(v.Magnitude == 5)

	local unit = v.Unit
	assert(math.abs(unit.X - 0.6) < 0.0001)
	assert(math.abs(unit.Y - 0.8) < 0.0001)
	assert(unit.Z == 0)

	-- Read-only
	asserterror(function()
		v.X = 10
	end)
	asserterror(function()
		v.Magnitude = 10
	end)
end

-- Methods
do
	-- Abs
	local absV = Vector3.new(-1, -2, 3):Abs()
	assert(absV.X == 1 and absV.Y == 2 and absV.Z == 3)

	-- Ceil
	local ceilV = Vector3.new(1.2, 2.7, -0.5):Ceil()
	assert(ceilV.X == 2 and ceilV.Y == 3 and ceilV.Z == 0)

	-- Floor
	local floorV = Vector3.new(1.7, 2.2, -0.5):Floor()
	assert(floorV.X == 1 and floorV.Y == 2 and floorV.Z == -1)

	-- Sign
	local signV = Vector3.new(-5, 0, 10):Sign()
	assert(signV.X == -1 and signV.Y == 0 and signV.Z == 1)

	-- Cross
	local cross = Vector3.xAxis:Cross(Vector3.yAxis)
	assert(math.abs(cross.Z - 1) < 0.0001)

	-- Dot
	assert(Vector3.xAxis:Dot(Vector3.yAxis) == 0)
	assert(Vector3.xAxis:Dot(Vector3.xAxis) == 1)
	assert(Vector3.new(1, 2, 3):Dot(Vector3.new(4, 5, 6)) == 32)

	-- Lerp
	local a = Vector3.new(0, 0, 0)
	local b = Vector3.new(10, 20, 30)
	local mid = a:Lerp(b, 0.5)
	assert(mid.X == 5 and mid.Y == 10 and mid.Z == 15)

	-- Max
	local maxV = Vector3.new(1, 5, 3):Max(Vector3.new(4, 2, 6))
	assert(maxV.X == 4 and maxV.Y == 5 and maxV.Z == 6)

	-- Min
	local minV = Vector3.new(1, 5, 3):Min(Vector3.new(4, 2, 6))
	assert(minV.X == 1 and minV.Y == 2 and minV.Z == 3)

	-- FuzzyEq
	local v1 = Vector3.new(1, 2, 3)
	local v2 = Vector3.new(1.000001, 2.000001, 3.000001)
	assert(v1:FuzzyEq(v2, 0.00001))
	assert(not v1:FuzzyEq(v2, 0.0000001))
end

-- Operators
do
	-- Addition
	local sum = Vector3.new(1, 2, 3) + Vector3.new(4, 5, 6)
	assert(sum.X == 5 and sum.Y == 7 and sum.Z == 9)

	-- Subtraction
	local diff = Vector3.new(5, 7, 9) - Vector3.new(1, 2, 3)
	assert(diff.X == 4 and diff.Y == 5 and diff.Z == 6)

	-- Multiplication (vector)
	local prod = Vector3.new(2, 3, 4) * Vector3.new(5, 6, 7)
	assert(prod.X == 10 and prod.Y == 18 and prod.Z == 28)

	-- Multiplication (scalar)
	local scaled1 = Vector3.new(1, 2, 3) * 2
	assert(scaled1.X == 2 and scaled1.Y == 4 and scaled1.Z == 6)

	local scaled2 = 3 * Vector3.new(1, 2, 3)
	assert(scaled2.X == 3 and scaled2.Y == 6 and scaled2.Z == 9)

	-- Division (vector)
	local quot = Vector3.new(10, 20, 30) / Vector3.new(2, 4, 5)
	assert(quot.X == 5 and quot.Y == 5 and quot.Z == 6)

	-- Division (scalar)
	local div = Vector3.new(6, 9, 12) / 3
	assert(div.X == 2 and div.Y == 3 and div.Z == 4)

	-- Unary minus
	local neg = -Vector3.new(1, -2, 3)
	assert(neg.X == -1 and neg.Y == 2 and neg.Z == -3)

	-- Equality
	assert(Vector3.new(1, 2, 3) == Vector3.new(1, 2, 3))
	assert(Vector3.new(1, 2, 3) ~= Vector3.new(1, 2, 4))
end

-- Type and string
do
	local v = Vector3.new(1, 2, 3)
	assert(typeof(v) == "Vector3")
	assert(tostring(v) == "1, 2, 3")
end
