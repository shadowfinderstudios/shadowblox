# Development

shadowblox is built using [SCons](https://www.scons.org/). It supports (without
comprehensive testing) the following toolchains:

- GCC (including MinGW)
- LLVM/Clang
- MSVC (Windows only)
- Clang-cl (Windows only)

with no particular restrictions on platform or architecture. C++20 or later is
required. If you experience compilation or runtime issues on any of these
toolchains or any (reasonably common) platform/architecture, please submit an
issue.

Build prerequisites:

- `scons`
- Your toolchain of choice
- Download the Roblox API dump using `scripts/download_dump` (or manually into
  `scripts/data/`)

The main shadowblox library is compiled into a statically-linked library. There
are only two SCons parameters:

- `toolchain`: `gcc`, `clang`, `msvc`, or `msvc-clang`
- `config`: `debug`, `relwithdbg`, `release`

For example, to build with 4 simultaneous jobs for MSVC in debug mode:

```
scons -j4 toolchain=msvc config=debug
```

A test binary is compiled in `shadowblox_tests/shadowblox_tests` (or
`shadowblox_tests\shadowblox_tests.exe` on Windows).
