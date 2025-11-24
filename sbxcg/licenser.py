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

import os
import re
import subprocess
import typing

from . import constants


def handle_ext(path: str, header: str, header_regex: str, f: typing.TextIO):
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
