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

import argparse

from . import api, licenser

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

# Codegen
gen_parser = subparsers.add_parser("gen", description="Code generation tools")
gen_subparsers = gen_parser.add_subparsers(dest="gen_subcommand", required=True)

# Licenser
licenser_parser = subparsers.add_parser(
    "licenser", description="License header checker"
)

args = parser.parse_args()

if args.subcommand == "api":
    if args.api_subcommand == "download":
        api.download_dump()
elif args.subcommand == "gen":
    pass
elif args.subcommand == "licenser":
    licenser.run()
