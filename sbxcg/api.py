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

import urllib.request
from pathlib import Path


def download_dump():
    data_path = Path(__file__).parent / "data"
    in_path = data_path / "studio_version.txt"
    out_path = data_path / "api_dump.json"

    with open(in_path, "r") as f:
        version = f.readline().strip()

    print(f"Studio version: {version}")

    urllib.request.urlretrieve(
        f"http://setup.roblox.com/{version}-API-Dump.json", out_path
    )

    print("Done!")
