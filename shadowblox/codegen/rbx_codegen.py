# SPDX-License-Identifier: MPL-2.0
################################################################################
# shadowblox - https://git.seki.pw/Fumohouse/shadowblox
#
# Copyright 2025-present ksk.
# Copyright 2025-present shadowblox contributors.
#
# This Source Code Form is subject to the terms of the Mozilla Public License,
# v. 2.0. If a copy of the MPL was not distributed with this file, You can
# obtain one at https://mozilla.org/MPL/2.0/.
#
# See COPYRIGHT.txt for more details.
################################################################################

import json
from pathlib import Path

from .enums import generate_enums

enum_header_path = "include/Sbx/DataTypes/EnumTypes.gen.hpp"
enum_source_path = "src/DataTypes/EnumTypes.gen.cpp"


def emit_files(target, source, env):
    in_files = [env.File("#sbxcg/data/api_dump.json")]

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
