# Example CMake snippet for a consuming Game Gem's Code/CMakeLists.txt.
#
# Also add "sqlgenCPP" to your Game Gem's gem.json "dependencies" array so O3DE
# processes sqlgenCPP before your Gem (making `Gem::sqlgenCPP.ORM` visible if enabled).

# ==============================================================================
# Pattern A: REQUIRED consumer (game cannot build without the C++26 ORM)
# ==============================================================================
# Configure your project with `-DSQLGENCPP_ORM_MODE=REQUIRED` so CMake fails
# immediately with exact missing prerequisites if the toolchain is unsupported.
#
# if(NOT TARGET Gem::sqlgenCPP.ORM)
#     message(FATAL_ERROR
#         "MyGame requires Gem::sqlgenCPP.ORM. Configure with -DSQLGENCPP_ORM_MODE=REQUIRED "
#         "on a supported GCC 16.1+ / CMake 4.5+ / Ninja toolchain.")
# endif()
#
# ly_add_target(
#     NAME MyGame.SaveStore STATIC
#     NAMESPACE Gem
#     SCAN_FOR_MODULES
#     CXX_STANDARD 26
#     FILES_CMAKE
#         mygame_savestore_orm_files.cmake   # lists SaveFacade.h + SaveFacadeOrm.cpp
#     TARGET_PROPERTIES
#         O3DE_PRIVATE_TARGET TRUE
#     INCLUDE_DIRECTORIES
#         PUBLIC
#             Source/Persistence
#     BUILD_DEPENDENCIES
#         PRIVATE
#             Gem::sqlgenCPP.ORM
# )

# ==============================================================================
# Pattern B: OPTIONAL consumer (builds with or without Gem::sqlgenCPP.ORM)
# ==============================================================================
if(TARGET Gem::sqlgenCPP.ORM)
    ly_add_target(
        NAME MyGame.SaveStore STATIC
        NAMESPACE Gem
        SCAN_FOR_MODULES
        CXX_STANDARD 26
        FILES_CMAKE
            mygame_savestore_orm_files.cmake   # lists SaveFacade.h + SaveFacadeOrm.cpp
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PUBLIC
                Source/Persistence
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::sqlgenCPP.ORM
    )
else()
    ly_add_target(
        NAME MyGame.SaveStore STATIC
        NAMESPACE Gem
        FILES_CMAKE
            mygame_savestore_stub_files.cmake  # lists SaveFacade.h + SaveFacadeStub.cpp
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PUBLIC
                Source/Persistence
    )
endif()

# Then link `Gem::MyGame.SaveStore` and `Gem::sqlgenCPP.API` into your ordinary
# C++20 (-fno-exceptions) Game Gem private target. Runtime callers query
# `MyGame::Persistence::IsOrmBackendAvailable()` (or you can add a compile-time
# macro via COMPILE_DEFINITIONS if you also need #if gating):
#
# ly_add_target(
#     NAME MyGame.Private.Object STATIC
#     ...
#     BUILD_DEPENDENCIES
#         PUBLIC
#             Gem::sqlgenCPP.API
#         PRIVATE
#             Gem::MyGame.SaveStore
# )
