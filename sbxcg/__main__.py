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

import argparse

from . import api, gen, licenser

####################
# Argument parsing #
####################

parser = argparse.ArgumentParser(
    prog="sbxcg", description="Code generation and API dump tooling for shadowblox"
)
subparsers = parser.add_subparsers(dest="subcommand", required=True)

# API
api_parser = subparsers.add_parser("api", description="API dump tools")
api_subparsers = api_parser.add_subparsers(dest="api_subcommand", required=True)

api_subparsers.add_parser("download", description="Download the API dump")

# Class list
classes_parser = subparsers.add_parser(
    "classes", description="Manage all generated classes"
)
classes_subparsers = classes_parser.add_subparsers(dest="classes_subcommand")

classes_subparsers.add_parser("update-all", description="Update all generated classes")

# Codegen
class_parser = subparsers.add_parser("class", description="Manage generated class")
class_parser.add_argument("name", type=str)

class_subparsers = class_parser.add_subparsers(dest="class_subcommand", required=True)
class_subparsers.add_parser(
    "generate", description="Generate or update the generated code"
)

## Members
class_member_parser = class_subparsers.add_parser(
    "member", description="Modify class members"
)
class_member_parser.add_argument("member", type=str)

class_member_parser.add_argument("--add", help="Add the member.", action="store_true")
class_member_parser.add_argument(
    "--remove", help="Remove the member.", action="store_true"
)

class_member_parser.add_argument(
    "--alias",
    help="For methods and properties: Use the same implementation as the given alias member.",
    type=str,
)

class_member_parser.add_argument(
    "--virtual", help="For methods: Mark the method as virtual.", action="store_true"
)
class_member_parser.add_argument(
    "--pure",
    help="For methods: Mark the method as pure virtual (i.e., abstract). Implies --virtual.",
    action="store_true",
)
class_member_parser.add_argument(
    "--const", help="For methods: Mark the method as const.", action="store_true"
)

class_member_parser.add_argument(
    "--custom",
    help="For methods: Use a Lua function rather than a C++ function.",
    action="store_true",
)
class_member_parser.add_argument(
    "--virtual-get",
    help="For properties: Mark the getter as virtual.",
    action="store_true",
)
class_member_parser.add_argument(
    "--virtual-set",
    help="For properties: Mark the setter as virtual.",
    action="store_true",
)

# Licenser
licenser_parser = subparsers.add_parser(
    "licenser", description="License header checker"
)

args = parser.parse_args()

if args.subcommand == "api":
    if args.api_subcommand == "download":
        api.download_dump()
elif args.subcommand == "classes":
    if args.classes_subcommand == "update-all":
        gen.update_all()
    elif args.classes_subcommand is None:
        gen.list()
elif args.subcommand == "class":
    if args.class_subcommand == "generate":
        gen.generate(args.name)
    elif args.class_subcommand == "member":
        gen.modify(
            args.name,
            args.member,
            {
                "alias": args.alias,
                "add": args.add,
                "remove": args.remove,
                "virtual": args.virtual,
                "pure": args.pure,
                "const": args.const,
                "custom": args.custom,
                "virtual_get": args.virtual_get,
                "virtual_set": args.virtual_set,
            },
        )
elif args.subcommand == "licenser":
    licenser.run()
