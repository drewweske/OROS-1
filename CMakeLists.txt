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

add_executable(oros_bootstrap)

target_sources(
    oros_bootstrap
    PRIVATE
        apps/bootstrap/main.cpp
)

target_compile_features(oros_bootstrap PRIVATE cxx_std_23)

target_compile_definitions(
    oros_bootstrap
    PRIVATE
        OROS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        OROS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        OROS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

if(MSVC)
    target_compile_options(
        oros_bootstrap
        PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /utf-8
    )

    if(OROS_WARNINGS_AS_ERRORS)
        target_compile_options(oros_bootstrap PRIVATE /WX)
    endif()
else()
    target_compile_options(
        oros_bootstrap
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
    )

    if(OROS_WARNINGS_AS_ERRORS)
        target_compile_options(oros_bootstrap PRIVATE -Werror)
    endif()
endif()

set_target_properties(
    oros_bootstrap
    PROPERTIES
        OUTPUT_NAME "Oros1"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)