# sqlgenCPP — O3DE Gem

A community [Open 3D Engine (O3DE)](https://github.com/o3de/o3de) Gem that brings [`CodeFirstCppDB_sqlgen`](https://github.com/mythrz/CodeFirstCppDB_sqlgen) — a C++26 module-based, reflection-driven Code-First ORM built on [`sqlgen`](https://github.com/munich-data-engineering/sqlgen) and [`reflect-cpp`](https://github.com/getml/reflect-cpp) — into O3DE, backed directly by O3DE's bundled **SQLite3 (`3rdParty::sqlite`)**.

---

## Features

- **Zero Duplicate SQLite:** Links directly against O3DE's bundled `3rdParty::sqlite` static library (no second system `libsqlite3.so` inside `Editor` or `GameLauncher`).
- **Clean C++20 / C++26 Boundary:** Exposes `Gem::sqlgenCPP.ORM` for opt-in C++26 module targets (`import std; import core; import dal;`) while keeping standard O3DE components, `Editor`, and `AzTest` harnesses on C++20 (`-fno-exceptions`).
- **O3DE Runtime Path Resolution:** Provides `sqlgenCPP::sqlgenCPPRequestBus` / `sqlgenCPPInterface` (`ResolveDatabasePath`, `GetDefaultDatabaseDirectory`, `IsCodeFirstOrmEnabled`) to resolve writable SQLite database paths under `@user@/Database` via `AZ::IO::FileIOBase`.
- **AzTest Unit Test Suite:** Includes `Gem::sqlgenCPP.Tests` verifying schema creation, FK topological sorting, initial database seeding, CRUD (`add_returning`, `try_find`, `update`, `remove`, `save_changes`), transaction rollback, WAL disk persistence, and `sqlgenCPPSystemComponent` path resolution.

---

## Requirements

- **O3DE Engine:** O3DE with opt-in C++20 module support in `ly_add_target` (`cmake/CxxModules.cmake`, `SCAN_FOR_MODULES`, `CXX_STANDARD`).
- **Compiler:** **GCC 16.1+** (required for C++26 `-freflection`, P2996/P3394 annotations, and `import std`).
- **Build System:** **CMake 4.5+** and **Ninja** (`Ninja` or `Ninja Multi-Config`).
- **Platform:** Linux (`x86_64`).

> **Note:** On compilers or generators without C++26 module support, the Gem still configures and builds its C++20 `sqlgenCPP` module cleanly with `IsCodeFirstOrmEnabled() == false` (skipping the C++26 ORM targets).

---

## Quick Start

### 1. Clone With Submodules
```bash
git clone --recurse-submodules https://github.com/mythrz/sqlgenCPP.git
```
If you already cloned without submodules:
```bash
git submodule update --init --recursive
```
*(Optional: for local ORM development, you can override the bundled submodule path at CMake configure time with `-DSQLGENCPP_CODEFIRST_DIR=/path/to/CodeFirstCppDB_sqlgen`.)*

### 2. Register and Enable the Gem in Your O3DE Project
```bash
<O3DE_ENGINE>/scripts/o3de.sh register -gp <PATH_TO_SQLGENCPP_GEM>
<O3DE_ENGINE>/scripts/o3de.sh enable-gem -gn sqlgenCPP -pp <PATH_TO_YOUR_PROJECT>
```

### 3. Build and Run O3DE Unit Tests
Configure your project with GCC 16+ and Ninja Multi-Config (`-DPAL_TRAIT_BUILD_TESTS_SUPPORTED=TRUE`), then build and run `sqlgenCPP.Tests`:
```bash
cmake --build <BUILD_DIR> --config profile --target AzTestRunner sqlgenCPP.Tests -j 2
<BUILD_DIR>/bin/profile/AzTestRunner <BUILD_DIR>/bin/profile/libsqlgenCPP.Tests.so AzRunUnitTests
```

---

## Using `sqlgenCPP` in Your Game Gem

To avoid mixing `#include <AzCore/...>` (C++20, `-fno-exceptions`) with `import std;` / `import dal;` (C++26, `-fexceptions`), keep your game's ORM entities and `DbContext` in a dedicated C++26 static sub-target behind a C++20 header facade:

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

> **Runtime Recommendations:**
> - Resolve writable save database paths via `sqlgenCPP::sqlgenCPPInterface::Get()->ResolveDatabasePath("save.db")`.
> - Enable `wal_mode = true` and a non-zero `busy_timeout` in `DAL::SQLiteConnectionOptions` for game workloads.
> - **Thread safety:** `DAL::DbContextBase`, `DAL::DbSet`, and `DAL::SQLiteConnection` are **not thread-safe**. Own one connection/context on a dedicated database worker thread and marshal callbacks back to the main thread via `AZ::TickBus::QueueFunction(...)`.
> - **Exception firewall:** Catch all C++ exceptions inside your C++26 database translation units before returning into O3DE's `-fno-exceptions` code.

---

## FAQ

**Is this an official project of `sqlgen` or O3DE?**  
No. This is an independent community Gem and is not affiliated with [`sqlgen`](https://github.com/munich-data-engineering/sqlgen) or [O3DE](https://github.com/o3de/o3de). Please report Gem-specific issues in this repository.

**Why is the Gem named `sqlgenCPP` instead of `sqlgen`?**  
To avoid both CMake target collisions (`ly_add_target(NAME sqlgen ...)` vs. upstream `add_library(sqlgen ...)`) and C++ namespace / header include collisions (`namespace sqlgen`, `<sqlgen/...>`).

---

## License

Dual-licensed under **Apache-2.0 OR MIT** (matching O3DE). See [LICENSE](LICENSE), [LICENSE_APACHE2.TXT](LICENSE_APACHE2.TXT), and [LICENSE_MIT.txt](LICENSE_MIT.txt) for details.
