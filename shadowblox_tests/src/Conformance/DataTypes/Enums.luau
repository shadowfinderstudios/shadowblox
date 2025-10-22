-- SPDX-License-Identifier: LGPL-3.0-or-later
--[============================================================================[
-- shadowblox - https://git.seki.pw/Fumohouse/shadowblox
--
-- Copyright 2025-present ksk.
-- Copyright 2025-present shadowblox contributors.
--
-- Licensed under the GNU Lesser General Public License version 3.0 or later.
-- See COPYRIGHT.txt for more details.
--
-- This program is free software: you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by the Free
-- Software Foundation, either version 3 of the License, or (at your option)
-- any later version.
--
-- This program is distributed in the hope that it will be useful, but WITHOUT
-- ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
-- FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
-- details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program. If not, see <https://www.gnu.org/licenses/>.
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
