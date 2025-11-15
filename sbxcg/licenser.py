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

import os
import re
import subprocess

from . import constants


def handle_ext(path, header, header_regex, f):
    contents = f.read()
    res = re.match(header_regex, contents)
    if res and res.group(0).strip() == header.strip():
        print("SKIP", path)
        return

    if res:
        contents = contents[res.end(0) :]

    f.seek(0)
    f.write(header + contents)
    f.truncate()
    print(path)


def run():
    root = (
        subprocess.check_output(["git", "rev-parse", "--show-toplevel"])
        .decode("utf-8")
        .strip()
    )
    os.chdir(root)

    files = [
        f
        for f in subprocess.check_output(
            ["git", "ls-files", "--cached", "--others", "--exclude-standard"]
        )
        .decode("utf-8")
        .split("\n")
        if f.endswith(".cpp")
        or f.endswith(".hpp")
        or f.endswith(".luau")
        or f.endswith(".py")
    ]

    for path in files:
        with open(path, "r+", encoding="utf-8") as f:
            if path.endswith(".cpp") or path.endswith(".hpp"):
                handle_ext(path, constants.header_cpp, constants.header_regex_cpp, f)
            elif path.endswith(".luau"):
                handle_ext(path, constants.header_luau, constants.header_regex_luau, f)
            elif path.endswith(".py"):
                handle_ext(path, constants.header_py, constants.header_regex_py, f)

    print("Done!")
