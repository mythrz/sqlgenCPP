
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/sqlgenCPP/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/sqlgenCPP/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Check to see if building the Gem Modules are supported for the current platform
if(NOT PAL_TRAIT_SQLGENCPP_SUPPORTED)
    return()
endif()

################################################################################
# CodeFirstCppDB_sqlgen (C++26 Reflection + Modules ORM backed by 3rdParty::sqlite)
################################################################################
set(SQLGENCPP_CODEFIRST_DIR "${CMAKE_CURRENT_LIST_DIR}/External/CodeFirstCppDB_sqlgen"
    CACHE PATH "Path to CodeFirstCppDB_sqlgen source directory (defaults to bundled submodule)")
set(SQLGENCPP_ORM_MODE "AUTO"
    CACHE STRING "CodeFirst C++26 ORM build mode: AUTO (enable if supported), REQUIRED (fail if unsupported), or OFF (path service only)")
set_property(CACHE SQLGENCPP_ORM_MODE PROPERTY STRINGS AUTO REQUIRED OFF)
option(SQLGENCPP_ENABLE_STACKTRACE "Enable Debug std::stacktrace capture in CodeFirstCppDB::Core" OFF)

string(TOUPPER "${SQLGENCPP_ORM_MODE}" _sqlgencpp_orm_mode_upper)
if(NOT _sqlgencpp_orm_mode_upper MATCHES "^(AUTO|REQUIRED|OFF)$")
    message(FATAL_ERROR "sqlgenCPP: Invalid SQLGENCPP_ORM_MODE='${SQLGENCPP_ORM_MODE}'. Expected AUTO, REQUIRED, or OFF.")
endif()

set(_sqlgencpp_orm_enabled FALSE)
if(_sqlgencpp_orm_mode_upper STREQUAL "OFF")
    message(STATUS "sqlgenCPP: CodeFirstCppDB_sqlgen C++26 ORM explicitly disabled (SQLGENCPP_ORM_MODE=OFF).")
else()
    set(_sqlgencpp_missing_prereqs "")
    if(NOT PAL_TRAIT_BUILD_CXX_MODULES_SUPPORTED)
        list(APPEND _sqlgencpp_missing_prereqs "PAL_TRAIT_BUILD_CXX_MODULES_SUPPORTED (requires O3DE engine with cmake/CxxModules.cmake, CMake >= 3.28, supported generator/compiler)")
    endif()
    if(NOT PAL_TRAIT_BUILD_CXX_IMPORT_STD_SUPPORTED)
        list(APPEND _sqlgencpp_missing_prereqs "PAL_TRAIT_BUILD_CXX_IMPORT_STD_SUPPORTED (requires CMake >= 4.5 and Ninja/Ninja Multi-Config)")
    endif()
    if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "16.0"))
        list(APPEND _sqlgencpp_missing_prereqs "GCC >= 16.0 (detected '${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}')")
    endif()
    if(NOT EXISTS "${SQLGENCPP_CODEFIRST_DIR}/CMakeLists.txt")
        list(APPEND _sqlgencpp_missing_prereqs "CodeFirstCppDB_sqlgen checkout at '${SQLGENCPP_CODEFIRST_DIR}'")
    else()
        if(NOT EXISTS "${SQLGENCPP_CODEFIRST_DIR}/extern/sqlgen/CMakeLists.txt")
            list(APPEND _sqlgencpp_missing_prereqs "initialized nested submodule 'extern/sqlgen' (run `git submodule update --init --recursive`)")
        endif()
        if(NOT EXISTS "${SQLGENCPP_CODEFIRST_DIR}/extern/reflect-cpp/CMakeLists.txt")
            list(APPEND _sqlgencpp_missing_prereqs "initialized nested submodule 'extern/reflect-cpp' (run `git submodule update --init --recursive`)")
        endif()
    endif()

    if(NOT _sqlgencpp_missing_prereqs)
        set(CODEFIRST_SQLITE_TARGET "3rdParty::sqlite" CACHE STRING "" FORCE)
        set(CODEFIRST_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(CODEFIRST_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(CODEFIRST_INCLUDE_SAMPLES OFF CACHE BOOL "" FORCE)
        set(CODEFIRST_BUILD_SHARED OFF CACHE BOOL "" FORCE)
        set(CODEFIRST_ENABLE_STACKTRACE ${SQLGENCPP_ENABLE_STACKTRACE} CACHE BOOL "" FORCE)

        add_subdirectory("${SQLGENCPP_CODEFIRST_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/CodeFirstCppDB" EXCLUDE_FROM_ALL)

        add_library(${gem_name}.ORM INTERFACE)
        add_library(Gem::${gem_name}.ORM ALIAS ${gem_name}.ORM)
        target_link_libraries(${gem_name}.ORM
            INTERFACE
                CodeFirstCppDB::Core
                CodeFirstCppDB::DAL
        )
        set(_sqlgencpp_orm_enabled TRUE)
    else()
        string(JOIN "; " _sqlgencpp_missing_str ${_sqlgencpp_missing_prereqs})
        if(_sqlgencpp_orm_mode_upper STREQUAL "REQUIRED")
            message(FATAL_ERROR "sqlgenCPP: SQLGENCPP_ORM_MODE=REQUIRED but prerequisites are missing: ${_sqlgencpp_missing_str}")
        else()
            message(STATUS "sqlgenCPP: CodeFirstCppDB_sqlgen C++26 ORM disabled (missing: ${_sqlgencpp_missing_str}). Building path service only.")
        endif()
        unset(_sqlgencpp_missing_str)
    endif()
    unset(_sqlgencpp_missing_prereqs)
endif()
unset(_sqlgencpp_orm_mode_upper)

# The ${gem_name}.API target declares the common interface that users of this gem should depend on in their targets
ly_add_target(
    NAME ${gem_name}.API INTERFACE
    NAMESPACE Gem
    FILES_CMAKE
        sqlgencpp_api_files.cmake
        ${pal_dir}/sqlgencpp_api_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
           AZ::AzCore
)

# The ${gem_name}.Private.Object target is an internal target
# It should not be used outside of this Gems CMakeLists.txt
ly_add_target(
    NAME ${gem_name}.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        sqlgencpp_private_files.cmake
        ${pal_dir}/sqlgencpp_private_files.cmake
    TARGET_PROPERTIES
        O3DE_PRIVATE_TARGET TRUE
    COMPILE_DEFINITIONS
        PRIVATE
            SQLGENCPP_ORM_ENABLED=$<IF:$<BOOL:${_sqlgencpp_orm_enabled}>,1,0>
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

# Here add ${gem_name} target, it depends on the Private Object library and Public API interface
ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        sqlgencpp_shared_files.cmake
        ${pal_dir}/sqlgencpp_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::${gem_name}.API
        PRIVATE
            Gem::${gem_name}.Private.Object
)

# Include the gem name into the Client Module source file
# for use with the AZ_DECLARE_MODULE_CLASS macro
# This is to allow renaming of the gem to also cause
# the CreateModuleClass_Gem_<gem-name> function which
# is used to bootstrap the gem in monolithic builds to link to the new gem name
ly_add_source_properties(
SOURCES
    Source/Clients/sqlgenCPPModule.cpp
PROPERTY COMPILE_DEFINITIONS
    VALUES
        O3DE_GEM_NAME=${gem_name}
        O3DE_GEM_VERSION=${gem_version})

# By default, we will specify that the above target ${gem_name} would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})

# For the Client and Server variants of ${gem_name} Gem, an alias to the ${gem_name}.API target will be made
ly_create_alias(NAME ${gem_name}.Clients.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)
ly_create_alias(NAME ${gem_name}.Servers.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)
ly_create_alias(NAME ${gem_name}.Unified.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)

# Add in CMake dependencies for each gem dependency listed in this gem's gem.json file
# for the Clients, Servers, Unified gem variants
o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Clients Servers Unified)

# If we are on a host platform, we want to add the host tools targets like the ${gem_name}.Editor MODULE target
# Do not comment out this if-block if your gem has an Assets folder
# Required by Assets Processor to know where to look for this-gem-assets
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    # The ${gem_name}.Editor.API target can be used by other gems that want to interact with the ${gem_name}.Editor module
    ly_add_target(
        NAME ${gem_name}.Editor.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            sqlgencpp_editor_api_files.cmake
            ${pal_dir}/sqlgencpp_editor_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
    )

    # The ${gem_name}.Editor.Private.Object target is an internal target
    # which is only to be used by this gems CMakeLists.txt and any subdirectories
    # Other gems should not use this target
    ly_add_target(
        NAME ${gem_name}.Editor.Private.Object STATIC
        NAMESPACE Gem
        FILES_CMAKE
            sqlgencpp_editor_private_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                ${gem_name}.Private.Object
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            sqlgencpp_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::${gem_name}.Editor.API
            PRIVATE
                Gem::${gem_name}.Editor.Private.Object
    )

    # Include the gem name into the Editor Module source file
    # for use with the AZ_DECLARE_MODULE_CLASS macro
    # This is to allow renaming of the gem to also cause
    # the CreateModuleClass_Gem_<gem-name> function which
    # is used to bootstrap the gem in monolithic builds to link to the new gem name
    ly_add_source_properties(
    SOURCES
        Source/Tools/sqlgenCPPEditorModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

    # By default, we will specify that the above target ${gem_name} would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)

    # For the Tools and Builders variants of ${gem_name} Gem, an alias to the ${gem_name}.Editor API target will be made
    ly_create_alias(NAME ${gem_name}.Tools.API NAMESPACE Gem TARGETS Gem::${gem_name}.Editor.API)
    ly_create_alias(NAME ${gem_name}.Builders.API NAMESPACE Gem TARGETS Gem::${gem_name}.Editor.API)

    # Add in CMake dependencies for each gem dependency listed in this gem's gem.json file
    # for the Tools and Builders gem variants
    o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Tools Builders)
endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for ${gem_name}.Tests
    if(PAL_TRAIT_SQLGENCPP_TEST_SUPPORTED)
        set(_sqlgencpp_test_extra_deps "")
        if(_sqlgencpp_orm_enabled)
            # C++26 module test implementation (isolated from AzTest headers)
            ly_add_target(
                NAME ${gem_name}.TestImpl STATIC
                NAMESPACE Gem
                SCAN_FOR_MODULES
                CXX_STANDARD 26
                FILES_CMAKE
                    sqlgencpp_testimpl_files.cmake
                TARGET_PROPERTIES
                    O3DE_PRIVATE_TARGET TRUE
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                        Include
                BUILD_DEPENDENCIES
                    PRIVATE
                        Gem::${gem_name}.ORM
            )
            list(APPEND _sqlgencpp_test_extra_deps Gem::${gem_name}.TestImpl)
        endif()

        # We support ${gem_name}.Tests on this platform (runs path-service tests even when ORM is disabled)
        ly_add_target(
            NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                sqlgencpp_tests_files.cmake
            COMPILE_DEFINITIONS
                PRIVATE
                    SQLGENCPP_ORM_ENABLED=$<IF:$<BOOL:${_sqlgencpp_orm_enabled}>,1,0>
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
                    Include
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::${gem_name}.Private.Object
                    ${_sqlgencpp_test_extra_deps}
        )
        unset(_sqlgencpp_test_extra_deps)

        # Add ${gem_name}.Tests to googletest
        ly_add_googletest(
            NAME Gem::${gem_name}.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_SQLGENCPP_EDITOR_TEST_SUPPORTED)
            # We support ${gem_name}.Editor.Tests on this platform, add ${gem_name}.Editor.Tests target which depends on
            # private ${gem_name}.Editor.Private.Object target
            ly_add_target(
                NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    sqlgencpp_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                        Include
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::${gem_name}.Editor.Private.Object
            )

            # Add ${gem_name}.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::${gem_name}.Editor.Tests
            )
        endif()
    endif()
endif()
