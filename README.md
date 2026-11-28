# sqlgenCPP — O3DE Gem

A community [Open 3D Engine (O3DE)](https://github.com/o3de/o3de) Gem that integrates [`CodeFirstCppDB_sqlgen`](https://github.com/mythrz/CodeFirstCppDB_sqlgen) — a C++26 module-based, reflection-driven Code-First ORM built on [`sqlgen`](https://github.com/munich-data-engineering/sqlgen) and [`reflect-cpp`](https://github.com/getml/reflect-cpp) — into O3DE, backed by O3DE's bundled **SQLite3 (`3rdParty::sqlite`)**.

> **Maturity Note:** Currently targeted at **Linux (`x86_64`) GCC 16.1+ C++26 evaluation and game prototyping**. The pinned [`CodeFirstCppDB_sqlgen@affc3dd`](https://github.com/mythrz/CodeFirstCppDB_sqlgen/commit/affc3dd0870ff10c0d1c6fe344bdd13d97d374cf) submodule includes project-qualified `codefirst.core` / `codefirst.dal` modules, the streaming strict prepared-statement SQLite reader, range-checked keys & tracked-PK guards, `INSERT ... RETURNING` hydration, `BEGIN IMMEDIATE` schema migrations, composite-PK + FK upserts, and gated `libstdc++exp` linkage. GitHub Actions CI runs reduced-host CMake contract & hygiene checks; full O3DE engine + `AzTestRunner` pairings are tracked in the dated matrix below.

---

## Features

- **Injected Engine SQLite (`3rdParty::sqlite`):** Verifies that `3rdParty::sqlite` exists before configuring `CodeFirstCppDB_sqlgen`, then passes `CODEFIRST_SQLITE_TARGET=3rdParty::sqlite` so the ORM links O3DE's bundled static SQLite3 archive rather than pulling a separate system `libsqlite3.so`. *(Verify both dynamic `ldd` and static symbol/link-map output on your final executable/Editor if other third-party Gems also ship SQLite.)*
- **Clean C++20 / C++26 Target Boundary:** Exposes `Gem::sqlgenCPP.ORM` for opt-in C++26 module targets (`import std; import codefirst.core; import codefirst.dal;`) while keeping standard O3DE components, `Editor`, and `AzTest` harnesses on C++20 (`-fno-exceptions`).
- **Explicit ORM Build Modes (`SQLGENCPP_ORM_MODE`):**
  - `AUTO` *(default)*: Enables `Gem::sqlgenCPP.ORM` when all prerequisites (including `3rdParty::sqlite`, O3DE module traits, GCC 16.1+, and initialized submodules) are present; logs exact missing prerequisites and falls back to the path service otherwise.
  - `REQUIRED`: Fails CMake configure with a clear `FATAL_ERROR` listing any missing prerequisites.
  - `OFF`: Explicitly builds the C++20 path service only (useful for testing the fallback path).
- **Containment-Checked O3DE Path Service:** `sqlgenCPP::sqlgenCPPRequestBus` / `sqlgenCPPInterface` provides `ResolveDatabasePath`, `GetDefaultDatabaseDirectory`, and `IsCodeFirstOrmEnabled`:
  - Enforces strict containment under `@user@/Database` (rejects empty paths, embedded NUL bytes, absolute paths, `..` traversal, symlink escapes, trailing directory separators, and pre-existing directories passed without a trailing slash) and returns a weakly-canonical native path.
  - Preserves `":memory:"`.
  - Never silently falls back to OS temporary storage when `AZ::IO::FileIOBase` is active; headless temp fallback requires explicit opt-in (`allowTempFallbackWhenNoFileIO = true`).
- **AzTest Unit Test Suite (`Gem::sqlgenCPP.Tests`):**
  - Built when O3DE tests are enabled (`PAL_TRAIT_BUILD_TESTS_SUPPORTED=TRUE`) on a test-supported platform (currently **Linux**, where `PAL_TRAIT_SQLGENCPP_TEST_SUPPORTED=TRUE`).
  - Path-service and `sqlgenCPPSystemComponent` tests (8 tests: compile-time ORM flag, lexical/directory rejection, symlink-escape rejection, existing-directory rejection, `EBus` activation, headless `:memory:`/temp fallback policy, live `AZ::IO::LocalFileIO` `@user@` alias, and unset-alias failure) build and run regardless of `SQLGENCPP_ORM_MODE` (including `OFF`).
  - When ORM is enabled, also runs 10 C++26 tests (18 tests total) covering schema creation + FK enforcement, initial data seeding, deferred Unit-of-Work `save_changes()`, transaction rollback, verified `PRAGMA journal_mode == "wal"` disk persistence, embedded-NUL typed `TEXT` round-trips, atomic batch `upsert_range` rollback, composite-PK + FK single-row and batch upserts, transactional `migrate_to` schema upgrades, and `SystemComponent` path + persistence.

---

## Requirements & Tested Compatibility Matrix

### Full C++26 ORM Mode (`SQLGENCPP_ORM_MODE=REQUIRED` or `AUTO`)
- **O3DE Engine:** Requires an O3DE build with opt-in C++20 module support (`cmake/CxxModules.cmake`, `SCAN_FOR_MODULES`, and `PRIVATE` `CXX_STANDARD` in `ly_add_target`) and O3DE's `3rdParty::sqlite` target.
  - *(Note: Upstream `o3de/o3de` `development` does not yet include `cmake/CxxModules.cmake` or `ly_add_target` module arguments. Against unmodified upstream O3DE, `SQLGENCPP_ORM_MODE=AUTO` builds the C++20 path service only and `SQLGENCPP_ORM_MODE=REQUIRED` fails configure with the missing trait names.)*
- **Compiler:** **GCC 16.1+** (tested with GCC 16.2.1 on Linux `x86_64`).
- **Build System:** **CMake 4.5+** and **Ninja** (`Ninja` or `Ninja Multi-Config`).
- **CMake Options:**
  - `SQLGENCPP_ORM_MODE=AUTO|REQUIRED|OFF` (default: `AUTO`)
  - `SQLGENCPP_ENABLE_STACKTRACE=ON|OFF` (default: `OFF` — forwarded to `CODEFIRST_ENABLE_STACKTRACE`; when `OFF` and `CODEFIRST_ENABLE_CONTRACTS=OFF`, Debug `Core::DbError` stack traces return empty strings and `libstdc++exp.a` is not linked).
  - `SQLGENCPP_LEGACY_MODULE_ALIASES=ON|OFF` (default: `OFF` — forwarded to `CODEFIRST_LEGACY_MODULE_ALIASES`; keep `OFF` so bare `core`/`dal` module names are not exported into consuming O3DE projects, or set `ON` for deprecated `import core; import dal;` shims).
  - `SQLGENCPP_CODEFIRST_DIR=<path>` (default: bundled submodule `Code/External/CodeFirstCppDB_sqlgen`).

### Verified Pairings (Dated)

| Date | Platform / Toolchain | O3DE Engine Commit | `CodeFirstCppDB_sqlgen` Pin | Modes & Verification |
|---|---|---|---|---|
| 2026-11-28 | Linux `x86_64`, GCC 16.2.1, CMake 4.5.0, Ninja Multi-Config | [`mythrz/o3de@ed33eb4c`](https://github.com/mythrz/o3de/commit/ed33eb4cdca78260c387f7c9e70549d13e146583) | [`affc3dd`](https://github.com/mythrz/CodeFirstCppDB_sqlgen/commit/affc3dd0870ff10c0d1c6fe344bdd13d97d374cf) | `REQUIRED`/`AUTO` (`profile`, `SQLGENCPP_LEGACY_MODULE_ALIASES=OFF`): 18/18 `sqlgenCPP.Tests` passed (normal & symlinked `$TMPDIR`) + `MinimalGameConsumer` (WAL-verified write/reopen & non-creating missing read) passed; 0 dynamic `libsqlite3.so` (`ldd`), single static `sqlite3_open_v2` (`nm`). `sqlgenCPP.TestImpl` built in `release` and `debug` (`SQLGENCPP_ENABLE_STACKTRACE=OFF` omits `libstdc++exp.a`; `ON` links it in `debug`). `OFF`: 8/8 `sqlgenCPP.Tests` + optional consumer stub passed. Reduced host CI: 4/4 contracts passed. |

---

## Quick Start

### 1. Clone With Submodules
```bash
git clone --recurse-submodules --shallow-submodules https://github.com/mythrz/sqlgenCPP.git
```
If you already cloned without submodules:
```bash
git submodule update --init --recursive --depth 1
```
*(Note: `sqlgen` and `reflect-cpp` each pin a `vcpkg` submodule upstream; `--shallow-submodules` / `--depth 1` significantly speeds up checkout. `vcpkg` is disabled by `CodeFirstCppDB_sqlgen` and is not used at build or runtime.)*

### 2. Register and Enable the Gem in Your O3DE Project
```bash
<O3DE_ENGINE>/scripts/o3de.sh register -gp <PATH_TO_SQLGENCPP_GEM>
<O3DE_ENGINE>/scripts/o3de.sh enable-gem -gn sqlgenCPP -pp <PATH_TO_YOUR_PROJECT>
```

### 3. Configure, Build, and Run O3DE Unit Tests
Configure your project against [`mythrz/o3de@ed33eb4c`](https://github.com/mythrz/o3de/commit/ed33eb4cdca78260c387f7c9e70549d13e146583) with GCC 16.1+, Ninja Multi-Config, `-DPAL_TRAIT_BUILD_TESTS_SUPPORTED=TRUE`, and `-DSQLGENCPP_ORM_MODE=REQUIRED`:

```bash
cmake -S <PROJECT_DIR> -B <BUILD_DIR_REQUIRED> -G "Ninja Multi-Config" \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DPAL_TRAIT_BUILD_TESTS_SUPPORTED=TRUE \
  -DSQLGENCPP_ORM_MODE=REQUIRED

cmake --build <BUILD_DIR_REQUIRED> --config profile --target AzTestRunner sqlgenCPP.Tests -j 2
<BUILD_DIR_REQUIRED>/bin/profile/AzTestRunner <BUILD_DIR_REQUIRED>/bin/profile/libsqlgenCPP.Tests.so AzRunUnitTests
```

*(Note: Keep compiler warnings visible in your own targets. If third-party or older engine targets fail under GCC 16 due to `-Werror`, pass `-DO3DE_EXTRA_C_FLAGS="-Wno-error" -DO3DE_EXTRA_CXX_FLAGS="-Wno-error"` rather than suppressing all warnings with `-w`.)*

To verify the fallback (path-service-only) mode, configure **and rebuild** (either in a separate build directory or before re-running `AzTestRunner`):

```bash
cmake -S <PROJECT_DIR> -B <BUILD_DIR_OFF> -G "Ninja Multi-Config" \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DPAL_TRAIT_BUILD_TESTS_SUPPORTED=TRUE \
  -DSQLGENCPP_ORM_MODE=OFF

cmake --build <BUILD_DIR_OFF> --config profile --target AzTestRunner sqlgenCPP.Tests -j 2
<BUILD_DIR_OFF>/bin/profile/AzTestRunner <BUILD_DIR_OFF>/bin/profile/libsqlgenCPP.Tests.so AzRunUnitTests
```

All 8 `SqlgenCPPPathServiceTest.*` tests still build and pass in `OFF` mode.

---

## Using `sqlgenCPP` in Your Game Gem

Keep your game's ORM entities and `DbContext` in a dedicated C++26 static sub-target behind a C++20 (`-fno-exceptions`) header façade so `#include <AzCore/...>` is never mixed with `import std;` / `import codefirst.dal;` (`-fexceptions`).

See the complete, runnable consumer pattern in [**`Examples/MinimalGameConsumer/`**](Examples/MinimalGameConsumer/README.md), which includes:
- [**`CMakeLists.snippet.cmake`**](Examples/MinimalGameConsumer/CMakeLists.snippet.cmake) — Both **Required** (`-DSQLGENCPP_ORM_MODE=REQUIRED`) and **Optional** (`if(TARGET Gem::sqlgenCPP.ORM)`) consumer target configurations.
- [**`SaveFacade.h`**](Examples/MinimalGameConsumer/SaveFacade.h) — C++20 `-fno-exceptions`-safe DTOs and `noexcept` façade declarations.
- [**`SaveFacadeOrm.cpp`**](Examples/MinimalGameConsumer/SaveFacadeOrm.cpp) — C++26 module implementation with `try { ... } catch (...)` exception firewall, WAL + busy timeout, upsert, and reopen-read.
- [**`SaveFacadeStub.cpp`**](Examples/MinimalGameConsumer/SaveFacadeStub.cpp) — C++20 fallback stub for optional consumers when `Gem::sqlgenCPP.ORM` is unavailable.
- [**`UsageInO3DEComponent.cpp`**](Examples/MinimalGameConsumer/UsageInO3DEComponent.cpp) — O3DE caller resolving `@user@/Database/Saves/profile.db`, handling `AZ::Failure` explicitly, writing, closing, and reopening.

> **Runtime & Boundary Guidelines:**
> - **Gem dependency order:** Add `"sqlgenCPP"` to your Game Gem's `gem.json` `"dependencies"` array so `Gem::sqlgenCPP.ORM` is created before your Gem's `Code/CMakeLists.txt` runs.
> - **Exception firewall:** Catch all C++ exceptions inside your C++26 database translation units before returning into O3DE's `-fno-exceptions` code, and keep both sides on the same compiler/standard-library ABI.
> - **Paths:** Resolve save paths via `sqlgenCPP::sqlgenCPPInterface::Get()->ResolveDatabasePath("Saves/profile.db")`. Handle `AZ::Failure` explicitly rather than falling back to temp storage for player saves.
> - **SQLite WAL & timeouts:** Set `wal_mode = true` and `busy_timeout = 5000ms` in `DAL::SQLiteConnectionOptions`, and verify `PRAGMA journal_mode` if WAL is required.
> - **Thread safety:** `DAL::DbContextBase`, `DAL::DbSet`, and `DAL::SQLiteConnection` are **not thread-safe**. Own one connection/context on a dedicated database worker thread and marshal callbacks back to the main thread via `AZ::TickBus::QueueFunction(...)`.

---

## FAQ

**Is this an official project of `sqlgen` or O3DE?**  
No. This is an independent community Gem and is not affiliated with [`sqlgen`](https://github.com/munich-data-engineering/sqlgen) or [O3DE](https://github.com/o3de/o3de). Please report Gem-specific issues in this repository.

**Why is the Gem named `sqlgenCPP` instead of `sqlgen`?**  
To avoid both CMake target collisions (`ly_add_target(NAME sqlgen ...)` vs. upstream `add_library(sqlgen ...)`) and C++ namespace / header include collisions (`namespace sqlgen`, `<sqlgen/...>`).

---

## License & Third-Party Notices

`sqlgenCPP` is dual-licensed under **Apache-2.0 OR MIT** (matching O3DE). See [LICENSE](LICENSE), [LICENSE_APACHE2.TXT](LICENSE_APACHE2.TXT), and [LICENSE_MIT.txt](LICENSE_MIT.txt) for details.

For bundled and linked third-party dependencies (`CodeFirstCppDB_sqlgen`, `sqlgen`, `reflect-cpp`, vendored `yyjson`, and O3DE `3rdParty::sqlite`), see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
