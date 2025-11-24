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

-- Enums
do
	assert(typeof(Enum) == "Enums")
	assert(tostring(Enum) == "Enums")

	assert(typeof(Enum.Axis) == "Enum")
	asserterror(function()
		return Enum.AOAOOAOAOAOAOAO
	end, "AOAOOAOAOAOAOAO is not a valid member of Enums")

	assert(typeof(Enum:GetEnums()) == "table")
	assert(#Enum:GetEnums() > 0)
	assert(typeof(Enum:GetEnums()[1]) == "Enum")
end

-- Enum
do
	local e = Enum.Axis
	assert(tostring(e) == "Axis")

	assert(typeof(e.X) == "EnumItem")

	assert(typeof(e:GetEnumItems()) == "table")
	assert(#e:GetEnumItems() > 0)
	assert(typeof(e:GetEnumItems()[1]) == "EnumItem")

	assert(e:FromName("X") == e.X)
	assert(e:FromName("AIHGHAIH") == nil)

	assert(e:FromValue(0) == e.X)
	assert(e:FromValue(192865) == nil)
end

-- EnumItem
do
	local i = Enum.Axis.X
	assert(tostring(i) == "Enum.Axis.X")

	assert(i.Name == "X")
	assert(i.Value == 0)
	assert(i.EnumType == Enum.Axis)

	asserterror(function()
		i.Name = "Y"
	end)
	asserterror(function()
		i.Value = 1
	end)
	asserterror(function()
		i.EnumType = nil
	end)

	assert(Enum.Axis:FromName("X") == i)

	local tb = {}
	tb[i] = 1234
	assert(tb[Enum.Axis:FromName("X")] == 1234)
end
