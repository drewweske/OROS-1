cmake_minimum_required(VERSION 3.30)

project(
    OROS1
    VERSION 0.1.0
    DESCRIPTION "A proprietary open-world simulation engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(
    OROS_WARNINGS_AS_ERRORS
    "Treat warnings in OROS-owned code as build errors"
    ON
)

function(oros_enable_warnings target_name)
    if(MSVC)
        target_compile_options(
            ${target_name}
            PRIVATE
                /W4
                /permissive-
                /Zc:__cplusplus
                /utf-8
        )

        if(OROS_WARNINGS_AS_ERRORS)
            target_compile_options(
                ${target_name}
                PRIVATE
                    /WX
            )
        endif()
    else()
        target_compile_options(
            ${target_name}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
        )

        if(OROS_WARNINGS_AS_ERRORS)
            target_compile_options(
                ${target_name}
                PRIVATE
                    -Werror
            )
        endif()
    endif()
endfunction()

add_library(
    oros_foundation
    STATIC
)

target_sources(
    oros_foundation
    PRIVATE
        engine/foundation/src/contract.cpp
        engine/foundation/src/log.cpp
)

target_include_directories(
    oros_foundation
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/engine/foundation/include"
)

target_compile_features(
    oros_foundation
    PUBLIC
        cxx_std_23
)

oros_enable_warnings(oros_foundation)

add_executable(oros_bootstrap)

target_sources(
    oros_bootstrap
    PRIVATE
        apps/bootstrap/main.cpp
)

target_link_libraries(
    oros_bootstrap
    PRIVATE
        oros_foundation
)

target_compile_features(
    oros_bootstrap
    PRIVATE
        cxx_std_23
)

target_compile_definitions(
    oros_bootstrap
    PRIVATE
        OROS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        OROS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        OROS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

oros_enable_warnings(oros_bootstrap)

set_target_properties(
    oros_bootstrap
    PROPERTIES
        OUTPUT_NAME "Oros1"
        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

include(CTest)

if(BUILD_TESTING)
    add_executable(oros_foundation_tests)

    target_sources(
        oros_foundation_tests
        PRIVATE
            tests/foundation/foundation_tests.cpp
    )

    target_link_libraries(
        oros_foundation_tests
        PRIVATE
            oros_foundation
    )

    target_compile_features(
        oros_foundation_tests
        PRIVATE
            cxx_std_23
    )

    oros_enable_warnings(oros_foundation_tests)

    set_target_properties(
        oros_foundation_tests
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY
                "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    )

    add_test(
        NAME oros.foundation
        COMMAND oros_foundation_tests
    )

    add_executable(oros_contract_violation_probe)

    target_sources(
        oros_contract_violation_probe
        PRIVATE
            tests/foundation/contract_violation_probe.cpp
    )

    target_link_libraries(
        oros_contract_violation_probe
        PRIVATE
            oros_foundation
    )

    target_compile_features(
        oros_contract_violation_probe
        PRIVATE
            cxx_std_23
    )

    oros_enable_warnings(
        oros_contract_violation_probe
    )

    set_target_properties(
        oros_contract_violation_probe
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY
                "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    )
endif()