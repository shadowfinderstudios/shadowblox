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

header_regex_cpp = r"// SPDX-License-Identifier: .*\n/\*[\s\S]*?\*/\n*"
header_cpp = """// SPDX-License-Identifier: MPL-2.0
/*******************************************************************************
 * shadowblox - https://git.seki.pw/Fumohouse/shadowblox
 *
 * Copyright 2025-present ksk.
 * Copyright 2025-present shadowblox contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See COPYRIGHT.txt for more details.
 ******************************************************************************/

"""

header_regex_luau = r"-- SPDX-License-Identifier: .*\n--\[=*\[[\s\S]*?\]=*\]\n*"
header_luau = """-- SPDX-License-Identifier: MPL-2.0
--[============================================================================[
-- shadowblox - https://git.seki.pw/Fumohouse/shadowblox
--
-- Copyright 2025-present ksk.
-- Copyright 2025-present shadowblox contributors.
--
-- This Source Code Form is subject to the terms of the Mozilla Public License,
-- v. 2.0. If a copy of the MPL was not distributed with this file, You can
-- obtain one at https://mozilla.org/MPL/2.0/.
--
-- See COPYRIGHT.txt for more details.
--]============================================================================]

"""

header_regex_py = r"# SPDX-License-Identifier: .*\n#{2,}\n[\s\S]*?#{2,}\n*"
header_py = """# SPDX-License-Identifier: MPL-2.0
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

"""

type_map = {
    # Primitives
    "null": {"type": "void"},
    "bool": {"type": "bool"},
    "int": {"type": "int"},
    "int64": {"type": "int64_t", "include": "<cstdint>"},
    "float": {"type": "float"},
    "double": {"type": "double"},
    "string": {"type": "const char *"},
    # Luau
    "Function": {"type": "Variant", "include": '"Sbx/Classes/Variant.hpp"'},
    "Dictionary": {"type": "Variant", "include": '"Sbx/Classes/Variant.hpp"'},
    "Array": {"type": "Variant", "include": '"Sbx/Classes/Variant.hpp"'},
    # Data types
    "RBXScriptSignal": {
        "type": "DataTypes::RBXScriptSignal",
        "include": '"Sbx/DataTypes/RBXScriptSignal.hpp"',
    },
    # Other
    "Variant": {"type": "Variant", "include": '"Sbx/Classes/Variant.hpp"'},
}

auto_tags = {"ReadOnly", "CustomLuaState", "NotScriptable"}
