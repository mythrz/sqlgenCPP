# Minimal Game Consumer Example

This directory shows the recommended integration pattern for consuming `sqlgenCPP` from an O3DE Game Gem without leaking C++26 module/exception settings into O3DE's C++20 (`-fno-exceptions`) targets.

## Files

| File | Language / Target | Purpose |
|---|---|---|
| [SaveFacade.h](SaveFacade.h) | C++20 header (`-fno-exceptions` safe) | Plain DTOs (`PlayerSaveProfile`) and `noexcept` façade API shared across the boundary. |
| [SaveFacadeOrm.cpp](SaveFacadeOrm.cpp) | C++26 static sub-target (`Gem::sqlgenCPP.ORM`) | `import std; import codefirst.core; import codefirst.dal;`, WAL + busy timeout options, `upsert` / `try_find`, and `try { ... } catch (...)` exception firewall. |
| [SaveFacadeStub.cpp](SaveFacadeStub.cpp) | C++20 static sub-target (optional fallback) | Clean runtime failure stub compiled when `Gem::sqlgenCPP.ORM` is unavailable (`SQLGENCPP_ORM_MODE=OFF` or unsupported toolchain). |
| [UsageInO3DEComponent.cpp](UsageInO3DEComponent.cpp) | C++20 O3DE Gem component (`Gem::sqlgenCPP.API`) | Resolves `"Saves/profile.db"` under `@user@/Database` via `sqlgenCPPInterface`, explicitly handles `AZ::Failure`, writes a save, closes it, and reopens it. |
| [CMakeLists.snippet.cmake](CMakeLists.snippet.cmake) | O3DE Gem `Code/CMakeLists.txt` | Shows both **Required** (`-DSQLGENCPP_ORM_MODE=REQUIRED`) and **Optional** (`if(TARGET Gem::sqlgenCPP.ORM)`) consumer target wiring. |

## Key Rules

1. **Declare `"sqlgenCPP"` in your Game Gem's `gem.json` `"dependencies"`** so `Gem::sqlgenCPP.ORM` is defined before your Gem's `Code/CMakeLists.txt` runs.
2. **Never `#include <AzCore/...>` in the C++26 `.cpp` target**, and always wrap every ORM entry point in `try { ... } catch (...)` so no C++ exception crosses into O3DE's `-fno-exceptions` binaries.
3. **Never pass `allowTempFallbackWhenNoFileIO = true` for real player saves** — handle `AZ::Failure` from `ResolveDatabasePath` explicitly (for example, when `@user@` is unset or a caller passes an existing directory name).
4. **Own one connection/context on a dedicated database worker thread** and marshal completion callbacks back to O3DE's main thread via `AZ::TickBus::QueueFunction`.
