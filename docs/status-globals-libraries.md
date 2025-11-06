# Implementation Status: Globals and Libraries

## Globals

Luau functions:

| Name             | Status | Remarks | As of            |
| ---------------- | ------ | ------- | ---------------- |
| `assert`         | 🇱      |         |                  |
| `collectgarbage` | ❌     |         |                  |
| `error`          | 🇱      |         |                  |
| `gcinfo`         | 🇱      |         |                  |
| `getfenv`        | 🇱      |         |                  |
| `getmetatable`   | 🇱      |         |                  |
| `ipairs`         | 🇱      |         |                  |
| `loadstring`     | ❌     |         |                  |
| `newproxy`       | 🇱      |         |                  |
| `next`           | 🇱      |         |                  |
| `pairs`          | 🇱      |         |                  |
| `pcall`          | 🇱      |         |                  |
| `print`          | ✅     |         | 2025-11-06 (698) |
| `rawequal`       | 🇱      |         |                  |
| `rawget`         | 🇱      |         |                  |
| `rawlen`         | 🇱      |         |                  |
| `rawset`         | 🇱      |         |                  |
| `require`        | ❌     |         |                  |
| `select`         | 🇱      |         |                  |
| `setfenv`        | 🇱      |         |                  |
| `setmetatable`   | 🇱      |         |                  |
| `tonumber`       | 🇱      |         |                  |
| `tostring`       | 🇱      |         |                  |
| `type`           | 🇱      |         |                  |
| `unpack`         | 🇱      |         |                  |
| `xpcall`         | 🇱      |         |                  |

| Implemented | Not planned | Not implemented | Total |
| ----------- | ----------- | --------------- | ----- |
| 1           | 0           | 25              | 26    |

Luau variables:

| Name       | Status | Remarks | As of |
| ---------- | ------ | ------- | ----- |
| `_G`       | 🇱      |         |       |
| `_VERSION` | 🇱      |         |       |

| Implemented | Not planned | Not implemented | Total |
| ----------- | ----------- | --------------- | ----- |
| 0           | 0           | 2               | 2     |

Roblox functions:

| Name              | Status | Remarks     | As of            |
| ----------------- | ------ | ----------- | ---------------- |
| `delay`           | ❌     |             |                  |
| `DebuggerManager` | ❌     |             |                  |
| `elapsedTime`     | ❌     |             |                  |
| `PluginManager`   | ⛔     | Studio only |                  |
| `printidentity`   | ❌     |             |                  |
| `settings`        | ⛔     | Studio only |                  |
| `spawn`           | ❌     |             |                  |
| `stats`           | ❌     |             |                  |
| `tick`            | ❌     |             |                  |
| `time`            | ❌     |             |                  |
| `typeof`          | 🇱      |             |                  |
| `UserSettings`    | ❌     |             |                  |
| `version`         | ❌     |             |                  |
| `wait`            | ✅     |             | 2025-11-03 (697) |
| `warn`            | ✅     |             | 2025-11-06 (698) |
| `ypcall`          | ❌     |             |                  |

| Implemented | Not planned | Not implemented | Total |
| ----------- | ----------- | --------------- | ----- |
| 2           | 2           | 12              | 16    |

Roblox variables:

| Name        | Status | Remarks     | As of            |
| ----------- | ------ | ----------- | ---------------- |
| `Enum`      | ✅     |             | 2025-10-21 (695) |
| `game`      | ❌     |             |                  |
| `plugin`    | ⛔     | Studio only |                  |
| `shared`    | ❌     |             |                  |
| `script`    | ❌     |             |                  |
| `workspace` | ❌     |             |                  |

| Implemented | Not planned | Not implemented | Total |
| ----------- | ----------- | --------------- | ----- |
| 1           | 1           | 4               | 6     |

- ✅: Implemented with test coverage or verified behavior
- 🏃‍➡️: In progress
- ❌: Not implemented
- ⛔: Not planned
- 🇱: Provided by Luau without modification (conformance to Roblox behavior is
  unknown)

## Libraries

| Name        | Status | Remarks | As of |
| ----------- | ------ | ------- | ----- |
| `bit32`     | 🇱      |         |       |
| `buffer`    | 🇱      |         |       |
| `coroutine` | 🇱      |         |       |
| `debug`     | 🇱      |         |       |
| `math`      | 🇱      |         |       |
| `os`        | 🇱      |         |       |
| `string`    | 🇱      |         |       |
| `table`     | 🇱      |         |       |
| `task`      | 🏃‍➡️     |         |       |
| `utf8`      | 🇱      |         |       |
| `vector`    | 🇱      |         |       |

| Implemented | Not planned | Not implemented | Total |
| ----------- | ----------- | --------------- | ----- |
| 0           | 0           | 11              | 11    |

- ✅: Implemented with test coverage or verified behavior
- 🏃‍➡️: In progress
- ❌: Not implemented
- ⛔: Not planned
- 🇱: Provided by Luau without modification (conformance to Roblox behavior is
  unknown; some functions may be missing)
