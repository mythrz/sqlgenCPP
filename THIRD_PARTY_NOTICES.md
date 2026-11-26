# Third-Party Notices

`sqlgenCPP` itself is dual-licensed under **Apache-2.0 OR MIT** (see [LICENSE](LICENSE), [LICENSE_APACHE2.TXT](LICENSE_APACHE2.TXT), and [LICENSE_MIT.txt](LICENSE_MIT.txt)).

When built with the C++26 ORM enabled (`SQLGENCPP_ORM_MODE=AUTO` or `REQUIRED`), `sqlgenCPP` compiles and/or links the following third-party components:

| Component | Version / Pin | License | Copyright / Notice | Upstream |
|---|---|---|---|---|
| **CodeFirstCppDB_sqlgen** | Submodule `Code/External/CodeFirstCppDB_sqlgen` | MIT | Copyright (c) 2025-2026 myth D. (mythrz) and CodeFirstCppDB_sqlgen contributors | https://github.com/mythrz/CodeFirstCppDB_sqlgen |
| **sqlgen** | Nested submodule `extern/sqlgen` | MIT | Copyright (c) 2025 Dr. Patrick Urbanke (劉自成) | https://github.com/munich-data-engineering/sqlgen |
| **reflect-cpp** | Nested submodule `extern/reflect-cpp` | MIT | Copyright (c) 2023-2025 Code17 GmbH | https://github.com/getml/reflect-cpp |
| **yyjson** *(vendored in reflect-cpp)* | `0.12.0` (`include/rfl/thirdparty/yyjson.h`) | MIT | Copyright (c) 2020 YaoYuan `<ibireme@gmail.com>` | https://github.com/ibireme/yyjson |
| **SQLite3** *(via O3DE `3rdParty::sqlite`)* | Provided by host O3DE engine | Public Domain (SQLite Blessing) | The author disclaims copyright to this source code. | https://sqlite.org/copyright.html |

### Notes for Distribution

- **GoogleTest (`BSD-3-Clause`):** Pinned inside `CodeFirstCppDB_sqlgen/extern/googletest` for standalone ORM tests only. `sqlgenCPP` sets `CODEFIRST_BUILD_TESTS=OFF` and does not build or link that submodule.
- **`vcpkg` submodules:** Pinned upstream by `sqlgen` and `reflect-cpp`, disabled (`SQLGEN_USE_VCPKG=OFF`) and unused by `sqlgenCPP`.
- **Before shipping a game or redistributed binary:** Verify the exact linked archives, shared libraries, enabled codecs, and SQLite provider in your final platform build (e.g., via `ldd` and link-map/symbol inspection) and include all applicable third-party license notices for your target distribution.
