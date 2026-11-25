# sqlgenCPP — O3DE Gem

A community [Open 3D Engine (O3DE)](https://github.com/o3de/o3de) Gem that integrates [`CodeFirstCppDB_sqlgen`](https://github.com/mythrz/CodeFirstCppDB_sqlgen) — a C++26 module-based, reflection-driven Code-First ORM built on [`sqlgen`](https://github.com/munich-data-engineering/sqlgen) and [`reflect-cpp`](https://github.com/getml/reflect-cpp) — into O3DE, backed by O3DE's bundled **SQLite3 (`3rdParty::sqlite`)**.

> **Maturity Note:** Currently targeted at **Linux (`x86_64`) GCC 16+ C++26 evaluation and game prototyping**. The pinned [`CodeFirstCppDB_sqlgen@8852620`](https://github.com/mythrz/CodeFirstCppDB_sqlgen/commit/88526202f9bce79e42ab091f533cbfd8c13563ef) submodule includes the strict prepared-statement typed SQLite reader, RAII transactional writes/batches, connection/tracker lifetime fixes, and transactional `PRAGMA user_version` schema migrations (`migrate_to`). This repository does not yet run automated GitHub Actions CI for the full O3DE engine build.

---

## Features

- **Injected Engine SQLite (`3rdParty::sqlite`):** Passes `CODEFIRST_SQLITE_TARGET=3rdParty::sqlite` into `CodeFirstCppDB_sqlgen` so the ORM links O3DE's bundled static SQLite3 library rather than pulling a separate system `libsqlite3.so` into the Gem. *(Verify final executable/Editor linkage with `ldd` if other third-party Gems also ship SQLite.)*
- **Clean C++20 / C++26 Target Boundary:** Exposes `Gem::sqlgenCPP.ORM` for opt-in C++26 module targets (`import std; import core; import dal;`) while keeping standard O3DE components, `Editor`, and `AzTest` harnesses on C++20 (`-fno-exceptions`).
- **Explicit ORM Build Modes (`SQLGENCPP_ORM_MODE`):**
  - `AUTO` *(default)*: Enables `Gem::sqlgenCPP.ORM` when all prerequisites are present; logs exact missing prerequisites and falls back to the path service otherwise.
  - `REQUIRED`: Fails CMake configure with a clear `FATAL_ERROR` listing any missing prerequisites.
  - `OFF`: Explicitly builds the C++20 path service only (useful for testing the fallback path).
- **Containment-Checked O3DE Path Service:** `sqlgenCPP::sqlgenCPPRequestBus` / `sqlgenCPPInterface` provides `ResolveDatabasePath`, `GetDefaultDatabaseDirectory`, and `IsCodeFirstOrmEnabled`:
  - Enforces strict containment under `@user@/Database` (rejects empty paths, absolute paths, and `..` directory traversal before creating directories).
  - Preserves `":memory:"`.
  - Never silently falls back to OS temporary storage when `AZ::IO::FileIOBase` is active; headless temp fallback requires explicit opt-in (`allowTempFallbackWhenNoFileIO = true`).
- **AzTest Unit Test Suite (`Gem::sqlgenCPP.Tests`):**
  - Path-service and `sqlgenCPPSystemComponent` tests (7 tests, including live `AZ::IO::LocalFileIO` `@user@` alias, unset-alias failure, symlink-escape rejection, directory-path rejection, and `EBus` activation checks) **always** build and run (even with `SQLGENCPP_ORM_MODE=OFF`).
  - When ORM is enabled, also runs 9 C++26 tests covering schema creation + FK enforcement, initial data seeding, deferred Unit-of-Work `save_changes()`, transaction rollback, verified `PRAGMA journal_mode == "wal"` disk persistence, embedded-NUL typed `TEXT` round-trips, atomic batch `upsert_range` rollback, transactional `migrate_to` schema upgrades, and `SystemComponent` path + persistence.

---

## Requirements & Tested Engine Pairing

### Full C++26 ORM Mode (`SQLGENCPP_ORM_MODE=REQUIRED` or `AUTO`)
- **O3DE Engine:** Requires an O3DE build with opt-in C++20 module support (`cmake/CxxModules.cmake`, `SCAN_FOR_MODULES`, and `PRIVATE` `CXX_STANDARD` in `ly_add_target`).
  - **Tested Engine Fork & Commit:** [`mythrz/o3de`](https://github.com/mythrz/o3de) (`development` @ [`ff890e8e8a`](https://github.com/mythrz/o3de/commit/ff890e8e8a1deb413de465a058e9d304e029dc64))
  - **Tested ORM Submodule Pin:** [`mythrz/CodeFirstCppDB_sqlgen`](https://github.com/mythrz/CodeFirstCppDB_sqlgen) (`main` @ [`8852620`](https://github.com/mythrz/CodeFirstCppDB_sqlgen/commit/88526202f9bce79e42ab091f533cbfd8c13563ef))
  - *(Note: Upstream `o3de/o3de` `development` does not yet include `cmake/CxxModules.cmake` or `ly_add_target` module arguments. Against unmodified upstream O3DE, `SQLGENCPP_ORM_MODE=AUTO` builds the C++20 path service only and `SQLGENCPP_ORM_MODE=REQUIRED` fails configure with the missing trait names.)*
- **Compiler:** **GCC 16.1+** (tested with GCC 16.2.1 on Linux `x86_64`).
- **Build System:** **CMake 4.5+** and **Ninja** (`Ninja` or `Ninja Multi-Config`).
- **CMake Options:**
  - `SQLGENCPP_ORM_MODE=AUTO|REQUIRED|OFF` (default: `AUTO`)
  - `SQLGENCPP_ENABLE_STACKTRACE=ON|OFF` (default: `OFF` — forwarded to `CODEFIRST_ENABLE_STACKTRACE`; when `OFF`, Debug `Core::DbError` stack traces return empty strings).
  - `SQLGENCPP_CODEFIRST_DIR=<path>` (default: bundled submodule `Code/External/CodeFirstCppDB_sqlgen`).

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
Configure your project against [`mythrz/o3de@ff890e8e8a`](https://github.com/mythrz/o3de/commit/ff890e8e8a1deb413de465a058e9d304e029dc64) with GCC 16+, Ninja Multi-Config, `-DPAL_TRAIT_BUILD_TESTS_SUPPORTED=TRUE`, and `-DSQLGENCPP_ORM_MODE=REQUIRED`:

```bash
cmake -S <PROJECT_DIR> -B <BUILD_DIR> -G "Ninja Multi-Config" \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DPAL_TRAIT_BUILD_TESTS_SUPPORTED=TRUE \
  -DSQLGENCPP_ORM_MODE=REQUIRED \
  -DO3DE_EXTRA_C_FLAGS="-w" -DO3DE_EXTRA_CXX_FLAGS="-w"

cmake --build <BUILD_DIR> --config profile --target AzTestRunner sqlgenCPP.Tests -j 2
<BUILD_DIR>/bin/profile/AzTestRunner <BUILD_DIR>/bin/profile/libsqlgenCPP.Tests.so AzRunUnitTests
```

To verify the fallback (path-service-only) mode, reconfigure with `-DSQLGENCPP_ORM_MODE=OFF` and run the same `AzTestRunner` command — all `SqlgenCPPPathServiceTest.*` tests still build and pass.

---

## Using `sqlgenCPP` in Your Game Gem

Keep your game's ORM entities and `DbContext` in a dedicated C++26 static sub-target behind a C++20 header facade so `#include <AzCore/...>` (`-fno-exceptions`) is never mixed with `import std;` / `import dal;` (`-fexceptions`):

```cmake
# Inside your Game Gem's Code/CMakeLists.txt
ly_add_target(
    NAME MyGame.Database STATIC
    NAMESPACE Gem
    SCAN_FOR_MODULES
    CXX_STANDARD 26
    FILES_CMAKE
        mygame_database_files.cmake
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::sqlgenCPP.ORM
)
```

Inside your C++26 implementation `.cpp` file:
```cpp
import std;
import core;
import dal;

struct [[=DAL::Schema::table{ std::define_static_string("Players") }]] Player
{
    [[=DAL::Schema::primary_key{ true }]] std::int32_t id = 0;
    [[=DAL::Schema::unique{}]] std::string handle;
    std::string display_name;
    std::int32_t level = 1;
};

struct GameDbContext : DAL::DbContextBase<DAL::SQLiteConnection>
{
    Set<Player> players{ this };

    explicit GameDbContext(std::shared_ptr<DAL::SQLiteConnection> conn)
        : DAL::DbContextBase<DAL::SQLiteConnection>(std::move(conn))
    {
    }
};
```

> **Runtime & Boundary Guidelines:**
> - **Exception firewall:** Catch all C++ exceptions inside your C++26 database translation units before returning into O3DE's `-fno-exceptions` code, and keep both sides on the same compiler/standard-library ABI.
> - **Paths:** Resolve save paths via `sqlgenCPP::sqlgenCPPInterface::Get()->ResolveDatabasePath("save.db")`. Handle `AZ::Failure` explicitly rather than falling back to temp storage for player saves.
> - **SQLite WAL & timeouts:** Set `wal_mode = true` and `busy_timeout = 5000ms` in `DAL::SQLiteConnectionOptions`, and verify `PRAGMA journal_mode` if WAL is required.
> - **Thread safety:** `DAL::DbContextBase`, `DAL::DbSet`, and `DAL::SQLiteConnection` are **not thread-safe**. Own one connection/context on a dedicated database worker thread and marshal callbacks back to the main thread via `AZ::TickBus::QueueFunction(...)`.

---

## FAQ

**Is this an official project of `sqlgen` or O3DE?**  
No. This is an independent community Gem and is not affiliated with [`sqlgen`](https://github.com/munich-data-engineering/sqlgen) or [O3DE](https://github.com/o3de/o3de). Please report Gem-specific issues in this repository.

**Why is the Gem named `sqlgenCPP` instead of `sqlgen`?**  
To avoid both CMake target collisions (`ly_add_target(NAME sqlgen ...)` vs. upstream `add_library(sqlgen ...)`) and C++ namespace / header include collisions (`namespace sqlgen`, `<sqlgen/...>`).

---

## License

Dual-licensed under **Apache-2.0 OR MIT** (matching O3DE). See [LICENSE](LICENSE), [LICENSE_APACHE2.TXT](LICENSE_APACHE2.TXT), and [LICENSE_MIT.txt](LICENSE_MIT.txt) for details.
