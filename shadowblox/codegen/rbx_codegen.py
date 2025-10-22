# SPDX-License-Identifier: LGPL-3.0-or-later
################################################################################
# shadowblox - https://git.seki.pw/Fumohouse/shadowblox
#
# Copyright 2025-present ksk.
# Copyright 2025-present shadowblox contributors.
#
# Licensed under the GNU Lesser General Public License version 3.0 or later.
# See COPYRIGHT.txt for more details.
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
################################################################################

import json
from pathlib import Path
from .enums import generate_enums

enum_header_path = "include/Sbx/DataTypes/EnumTypes.gen.hpp"
enum_source_path = "src/DataTypes/EnumTypes.gen.cpp"


def emit_files(target, source, env):
    in_files = [env.File("#scripts/data/api_dump.json")]

    out_files = [
        env.File(enum_header_path),
        env.File(enum_source_path),
    ]

    env.Clean(out_files, target)

    return target + out_files, source + in_files


def generate_bindings(target, source, env):
    api = None
    with open(str(source[0])) as api_file:
        api = json.load(api_file)

    base_path = Path(__file__).parent / ".."

    generate_enums(api, base_path / enum_header_path, base_path / enum_source_path)

    return None
