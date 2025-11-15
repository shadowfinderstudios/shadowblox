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
