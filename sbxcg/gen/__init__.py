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

import glob
from pathlib import Path

from .. import utils
from . import templater

shadowblox_dir = (Path(__file__).parent / ".." / ".." / "shadowblox").resolve()
include_dir = shadowblox_dir / "include" / "Sbx" / "Classes"
src_dir = shadowblox_dir / "src" / "Classes"


def scan():
    scan = {}

    for header in glob.iglob(str(include_dir / "**" / "*.hpp"), recursive=True):
        with open(header, "r") as f:
            res = templater.parse_header(f.read())

        if res is None:
            continue

        name, config, blocks = res
        if name not in scan:
            scan[name] = []
        scan[name].append((header, config, blocks))

    for source in glob.iglob(str(src_dir / "**" / "*.cpp"), recursive=True):
        with open(source, "r") as f:
            res = templater.parse_source(f.read())

        if res is None:
            continue

        name, blocks = res
        if name in scan:
            scan[name].append((source, blocks))

    classes = {}

    for name, data in scan.items():
        if len(data) != 2:
            continue

        (header, config, header_blocks), (source, source_blocks) = data
        classes[name] = {
            "header": str(Path(header).relative_to(include_dir)).replace("\\", "/"),
            "source": str(Path(source).relative_to(src_dir)).replace("\\", "/"),
            "config": config,
            "header_blocks": header_blocks,
            "source_blocks": source_blocks,
        }

    return classes


def list():
    for name, data in scan().items():
        print(f"{name} - {data['header']}/{data['source']}")


def _update(dump_class: dict, scan_class: dict, classes: dict):
    name = dump_class["Name"]

    header, source = templater.generate(dump_class, scan_class, classes)

    header_out = include_dir / scan_class.get("header", f"{name}.hpp")
    source_out = src_dir / scan_class.get("source", f"{name}.cpp")

    with open(header_out, "w") as f:
        f.write(header)
    print(header_out)
    with open(source_out, "w") as f:
        f.write(source)
    print(source_out)


def update_all():
    dump = utils.load_dump()
    classes = scan()

    for name, scan_class in classes.items():
        dump_class = [c for c in dump["Classes"] if c["Name"] == name]
        if len(dump_class) == 0:
            print(f"WARNING: Class {name} does not exist anymore.")
            continue
        dump_class = dump_class[0]

        _update(dump_class, scan_class, classes)

    print("Done!")


def generate(name: str):
    dump = utils.load_dump()
    dump_class = [c for c in dump["Classes"] if c["Name"] == name]
    if len(dump_class) == 0:
        print(f"ERROR: Class {name} does not exist.")
        return
    dump_class = dump_class[0]

    classes = scan()
    scan_class = classes.get(name, {})

    _update(dump_class, scan_class, classes)
    print("Done!")


def modify(class_name: str, member_name: str, options: dict):
    dump = utils.load_dump()
    dump_class = [c for c in dump["Classes"] if c["Name"] == class_name]
    if len(dump_class) == 0:
        print(f"ERROR: Class {class_name} does not exist.")
        return
    dump_class = dump_class[0]

    classes = scan()
    scan_class = classes.get(class_name)
    if scan_class is None:
        print(f"ERROR: Class {class_name} has not been generated.")
        return
    if "config" not in scan_class:
        scan_class["config"] = {}
    config = scan_class["config"]

    dump_member = [m for m in dump_class["Members"] if m["Name"] == member_name]
    if len(dump_member) == 0:
        print(f"ERROR: Member {member_name} does not exist.")
        return
    dump_member = dump_member[0]

    if options["remove"]:
        if "members" not in config or member_name not in config["members"]:
            print(f"WARNING: Member {member_name} already not present.")
            return

        del config["members"][member_name]
        if len(config["members"]) == 0:
            del config["members"]
    else:
        if not options["add"] and (
            "members" not in config or member_name not in config["members"]
        ):
            print(f"ERROR: Member {member_name} not present; specify --add to add it.")
            return

        if "members" not in config:
            config["members"] = {}

        config["members"][member_name] = {}

        if options["alias"] is not None:
            config["members"][member_name]["alias"] = options["alias"]
        elif dump_member["MemberType"] == "Function":
            if options["custom"]:
                config["members"][member_name]["custom"] = True
            else:
                if options["virtual"]:
                    config["members"][member_name]["virtual"] = True
                if options["pure"]:
                    config["members"][member_name]["virtual"] = True
                    config["members"][member_name]["pure"] = True
                if options["const"]:
                    config["members"][member_name]["const"] = True
        elif dump_member["MemberType"] == "Property":
            if options["virtual_get"]:
                config["members"][member_name]["virtual_get"] = True
            if options["virtual_set"]:
                config["members"][member_name]["virtual_set"] = True

    _update(dump_class, scan_class, classes)
    print("Done!")
