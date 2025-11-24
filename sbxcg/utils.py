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

dump = None


def load_dump():
    global dump
    if dump is None:
        dump_path = Path(__file__).parent / "data" / "api_dump.json"
        with open(dump_path, "rb") as f:
            dump = json.load(f)
    return dump
