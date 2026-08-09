cmake_minimum_required(VERSION 3.30)

project(
    OROS1
    VERSION 0.1.0
    DESCRIPTION "A proprietary open-world simulation engine"
    LANGUAGES CXX
)

find_package(Threads REQUIRED)

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

function(
    oros_add_test_target
    target_name
    test_name
    source_file
)
    add_executable(
        ${target_name}
    )

    target_sources(
        ${target_name}
        PRIVATE
            ${source_file}
    )

    target_link_libraries(
        ${target_name}
        PRIVATE
            ${ARGN}
    )

    target_compile_features(
        ${target_name}
        PRIVATE
            cxx_std_23
    )

    oros_enable_warnings(
        ${target_name}
    )

    set_target_properties(
        ${target_name}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY
                "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    )

    add_test(
        NAME ${test_name}
        COMMAND ${target_name}
    )
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

oros_enable_warnings(
    oros_foundation
)

add_library(
    oros_platform
    STATIC
)

target_sources(
    oros_platform
    PRIVATE
        engine/platform/src/windows/window.cpp
)

target_include_directories(
    oros_platform
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/engine/platform/include"
)

target_link_libraries(
    oros_platform
    PUBLIC
        oros_foundation
    PRIVATE
        user32
)

target_compile_features(
    oros_platform
    PUBLIC
        cxx_std_23
)

target_compile_definitions(
    oros_platform
    PRIVATE
        UNICODE
        _UNICODE
)

oros_enable_warnings(
    oros_platform
)

add_library(
    oros_rendering
    STATIC
)

target_sources(
    oros_rendering
    PRIVATE
        engine/rendering/src/d3d12/renderer.cpp
)

target_include_directories(
    oros_rendering
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/engine/rendering/include"
)

target_link_libraries(
    oros_rendering
    PUBLIC
        oros_foundation
    PRIVATE
        d3d12
        dxgi
        user32
)

target_compile_features(
    oros_rendering
    PUBLIC
        cxx_std_23
)

target_compile_definitions(
    oros_rendering
    PRIVATE
        UNICODE
        _UNICODE
)

oros_enable_warnings(
    oros_rendering
)

add_library(
    oros_runtime
    STATIC
)

target_sources(
    oros_runtime
    PRIVATE
        engine/runtime/src/engine_runtime.cpp
        engine/runtime/src/fixed_step_scheduler.cpp
)

target_include_directories(
    oros_runtime
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/engine/runtime/include"
)

target_link_libraries(
    oros_runtime
    PUBLIC
        oros_foundation
)

target_compile_features(
    oros_runtime
    PUBLIC
        cxx_std_23
)

oros_enable_warnings(
    oros_runtime
)

add_library(
    oros_world
    STATIC
)

target_sources(
    oros_world
    PRIVATE
        engine/world/src/entity_id.cpp
        engine/world/src/world.cpp
        engine/world/src/world_position.cpp
)

target_include_directories(
    oros_world
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/engine/world/include"
)

target_link_libraries(
    oros_world
    PUBLIC
        oros_foundation
)

target_compile_features(
    oros_world
    PUBLIC
        cxx_std_23
)

oros_enable_warnings(
    oros_world
)

add_library(
    oros_physics
    STATIC
)

target_sources(
    oros_physics
    PRIVATE
        engine/physics/src/axis_aligned_bounds.cpp
        engine/physics/src/broad_phase.cpp
        engine/physics/src/broad_phase_pair.cpp
        engine/physics/src/broad_phase_proxy.cpp
        engine/physics/src/capsule_box_narrow_phase.cpp
        engine/physics/src/capsule_capsule_narrow_phase.cpp
        engine/physics/src/collider_geometry.cpp
        engine/physics/src/collider_id.cpp
        engine/physics/src/collision_contact.cpp
        engine/physics/src/collision_shape.cpp
        engine/physics/src/collision_shape_bounds.cpp
        engine/physics/src/narrow_phase.cpp
        engine/physics/src/narrow_phase_dispatch.cpp
        engine/physics/src/physics_vector.cpp
        engine/physics/src/sphere_capsule_narrow_phase.cpp
)

target_include_directories(
    oros_physics
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/engine/physics/include"
)

target_link_libraries(
    oros_physics
    PUBLIC
        oros_foundation
        oros_world
)

target_compile_features(
    oros_physics
    PUBLIC
        cxx_std_23
)

oros_enable_warnings(
    oros_physics
)

add_library(
    oros_assets
    STATIC
)

target_sources(
    oros_assets
    PRIVATE
        engine/assets/src/asset_catalog.cpp
        engine/assets/src/asset_cook_pipeline.cpp
        engine/assets/src/asset_cooker_registry.cpp
        engine/assets/src/asset_hot_reload_pipeline.cpp
        engine/assets/src/asset_id.cpp
        engine/assets/src/asset_import_pipeline.cpp
        engine/assets/src/asset_importer_registry.cpp
        engine/assets/src/asset_load_pipeline.cpp
        engine/assets/src/binary_asset_cooker.cpp
        engine/assets/src/binary_asset_importer.cpp
        engine/assets/src/content_hash.cpp
        engine/assets/src/cooked_asset_artifact_pipeline.cpp
        engine/assets/src/cooked_asset_artifact_serialization.cpp
        engine/assets/src/cooked_asset_artifact_store.cpp
        engine/assets/src/cooked_asset_cache.cpp
        engine/assets/src/resource_handle.cpp
        engine/assets/src/run_length_compression_codec.cpp
)

target_include_directories(
    oros_assets
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/engine/assets/include"
)

target_link_libraries(
    oros_assets
    PUBLIC
        oros_foundation
)

target_compile_features(
    oros_assets
    PUBLIC
        cxx_std_23
)

oros_enable_warnings(
    oros_assets
)

add_library(
    oros_streaming
    STATIC
)

target_sources(
    oros_streaming
    PRIVATE
        engine/streaming/src/world_cell_io_request.cpp
        engine/streaming/src/world_cell_io_service.cpp
        engine/streaming/src/world_cell_io_shared_state.cpp
        engine/streaming/src/world_cell_key.cpp
        engine/streaming/src/world_cell_persistence_format.cpp
        engine/streaming/src/world_cell_persistence_store.cpp
        engine/streaming/src/world_cell_residency.cpp
        engine/streaming/src/world_cell_revision_id.cpp
        engine/streaming/src/world_cell_snapshot.cpp
        engine/streaming/src/world_cell_snapshot_manifest.cpp
)

target_include_directories(
    oros_streaming
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/engine/streaming/include"
)

target_link_libraries(
    oros_streaming
    PUBLIC
        oros_foundation
        oros_world
    PRIVATE
        Threads::Threads
)

target_compile_features(
    oros_streaming
    PUBLIC
        cxx_std_23
)

oros_enable_warnings(
    oros_streaming
)

add_library(
    oros_physical_world
    STATIC
)

target_sources(
    oros_physical_world
    PRIVATE
        engine/physical_world/src/world_capsule_contact_normal.cpp
        engine/physical_world/src/world_capsule_contact_query.cpp
        engine/physical_world/src/world_capsule_depenetration.cpp
        engine/physical_world/src/world_first_person_controller.cpp
        engine/physical_world/src/world_capsule_ground_motion.cpp
        engine/physical_world/src/world_capsule_ground_query.cpp
        engine/physical_world/src/world_capsule_ground_snap.cpp
        engine/physical_world/src/world_capsule_motion.cpp
        engine/physical_world/src/world_capsule_traversal_settings.cpp
        engine/physical_world/src/world_cell_collider_payload.cpp
        engine/physical_world/src/world_cell_collider_registry.cpp
        engine/physical_world/src/world_cell_collider_set.cpp
)

target_include_directories(
    oros_physical_world
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/engine/physical_world/include"
)

target_link_libraries(
    oros_physical_world
    PUBLIC
        oros_foundation
        oros_physics
        oros_streaming
        oros_world
)

target_compile_features(
    oros_physical_world
    PUBLIC
        cxx_std_23
)

oros_enable_warnings(
    oros_physical_world
)

add_library(
    oros_ai
    STATIC
)

target_sources(
    oros_ai
    PRIVATE
        engine/ai/src/actor_activity_intent_key.cpp
        engine/ai/src/actor_faction_membership.cpp
        engine/ai/src/actor_faction_membership_set.cpp
        engine/ai/src/actor_faction_reputation.cpp
        engine/ai/src/actor_faction_reputation_set.cpp
        engine/ai/src/faction_key.cpp
        engine/ai/src/faction_relationship.cpp
        engine/ai/src/faction_relationship_matrix.cpp
        engine/ai/src/faction_trust.cpp
        engine/ai/src/faction_trust_matrix.cpp
        engine/ai/src/actor_schedule.cpp
        engine/ai/src/actor_schedule_execution_state.cpp
        engine/ai/src/actor_scheduled_activity.cpp
        engine/ai/src/actor_schedule_window.cpp
        engine/ai/src/actor_simulation_fidelity_registry.cpp
        engine/ai/src/actor_simulation_fidelity_state.cpp
        engine/ai/src/actor_simulation_position_snapshot_set.cpp
        engine/ai/src/actor_simulation_focus_decision.cpp
        engine/ai/src/actor_simulation_focus_decision_set.cpp
        engine/ai/src/actor_simulation_focus_decision_application.cpp
        engine/ai/src/actor_simulation_focus_decision_set_application.cpp
        engine/ai/src/actor_simulation_focus_policy.cpp
        engine/ai/src/actor_simulation_focus_source_registry.cpp
)

target_include_directories(
    oros_ai
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/engine/ai/include"
)

target_link_libraries(
    oros_ai
    PUBLIC
        oros_foundation
        oros_world
)

target_compile_features(
    oros_ai
    PUBLIC
        cxx_std_23
)

oros_enable_warnings(
    oros_ai
)

add_executable(
    oros_bootstrap
)

target_sources(
    oros_bootstrap
    PRIVATE
        apps/bootstrap/live_player_demo.cpp
        apps/bootstrap/main.cpp
        apps/bootstrap/physical_world_demo.cpp
)

target_link_libraries(
    oros_bootstrap
    PRIVATE
        oros_foundation
        oros_physical_world
        oros_physics
        oros_platform
        oros_rendering
        oros_runtime
        oros_streaming
        oros_world
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

oros_enable_warnings(
    oros_bootstrap
)

set_target_properties(
    oros_bootstrap
    PROPERTIES
        OUTPUT_NAME "Oros1"
        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

include(CTest)

if(BUILD_TESTING)
    oros_add_test_target(
        oros_foundation_tests
        oros.foundation
        tests/foundation/foundation_tests.cpp
        oros_foundation
    )

    add_executable(
        oros_contract_violation_probe
    )

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

    oros_add_test_target(
        oros_platform_input_tests
        oros.platform.input
        tests/platform/input_tests.cpp
        oros_platform
    )

    oros_add_test_target(
        oros_platform_window_tests
        oros.platform.window
        tests/platform/window_tests.cpp
        oros_platform
    )

    oros_add_test_target(
        oros_rendering_renderer_tests
        oros.rendering.renderer
        tests/rendering/renderer_tests.cpp
        oros_platform
        oros_rendering
    )

    oros_add_test_target(
        oros_runtime_fixed_step_scheduler_tests
        oros.runtime.fixed_step_scheduler
        tests/runtime/fixed_step_scheduler_tests.cpp
        oros_runtime
    )

    oros_add_test_target(
        oros_runtime_engine_runtime_tests
        oros.runtime.engine_runtime
        tests/runtime/engine_runtime_tests.cpp
        oros_runtime
    )

    oros_add_test_target(
        oros_world_entity_id_tests
        oros.world.entity_id
        tests/world/entity_id_tests.cpp
        oros_world
    )

    oros_add_test_target(
        oros_world_component_store_tests
        oros.world.component_store
        tests/world/component_store_tests.cpp
        oros_world
    )

    oros_add_test_target(
        oros_world_position_tests
        oros.world.position
        tests/world/world_position_tests.cpp
        oros_world
    )

    oros_add_test_target(
        oros_world_time_tests
        oros.world.time
        tests/world/world_time_tests.cpp
        oros_world
    )

    oros_add_test_target(
        oros_world_tests
        oros.world
        tests/world/world_tests.cpp
        oros_world
    )

    oros_add_test_target(
        oros_physics_vector_tests
        oros.physics.vector
        tests/physics/physics_vector_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_collider_id_tests
        oros.physics.collider_id
        tests/physics/collider_id_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_collider_geometry_tests
        oros.physics.collider_geometry
        tests/physics/collider_geometry_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_collision_contact_tests
        oros.physics.collision_contact
        tests/physics/collision_contact_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_collision_shape_tests
        oros.physics.collision_shape
        tests/physics/collision_shape_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_axis_aligned_bounds_tests
        oros.physics.axis_aligned_bounds
        tests/physics/axis_aligned_bounds_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_collision_shape_bounds_tests
        oros.physics.collision_shape_bounds
        tests/physics/collision_shape_bounds_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_broad_phase_tests
        oros.physics.broad_phase
        tests/physics/broad_phase_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_broad_phase_pair_tests
        oros.physics.broad_phase_pair
        tests/physics/broad_phase_pair_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_broad_phase_proxy_tests
        oros.physics.broad_phase_proxy
        tests/physics/broad_phase_proxy_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_narrow_phase_tests
        oros.physics.narrow_phase
        tests/physics/narrow_phase_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_narrow_phase_dispatch_tests
        oros.physics.narrow_phase_dispatch
        tests/physics/narrow_phase_dispatch_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_sphere_box_narrow_phase_tests
        oros.physics.sphere_box_narrow_phase
        tests/physics/sphere_box_narrow_phase_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_box_box_narrow_phase_tests
        oros.physics.box_box_narrow_phase
        tests/physics/box_box_narrow_phase_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_sphere_capsule_narrow_phase_tests
        oros.physics.sphere_capsule_narrow_phase
        tests/physics/sphere_capsule_narrow_phase_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_capsule_box_narrow_phase_tests
        oros.physics.capsule_box_narrow_phase
        tests/physics/capsule_box_narrow_phase_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_physics_capsule_capsule_narrow_phase_tests
        oros.physics.capsule_capsule_narrow_phase
        tests/physics/capsule_capsule_narrow_phase_tests.cpp
        oros_physics
    )

    oros_add_test_target(
        oros_assets_asset_catalog_tests
        oros.assets.asset_catalog
        tests/assets/asset_catalog_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_asset_compression_codec_tests
        oros.assets.asset_compression_codec
        tests/assets/asset_compression_codec_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_asset_cook_pipeline_tests
        oros.assets.asset_cook_pipeline
        tests/assets/asset_cook_pipeline_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_asset_cooker_tests
        oros.assets.asset_cooker
        tests/assets/asset_cooker_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_asset_cooker_registry_tests
        oros.assets.asset_cooker_registry
        tests/assets/asset_cooker_registry_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_asset_hot_reload_pipeline_tests
        oros.assets.asset_hot_reload_pipeline
        tests/assets/asset_hot_reload_pipeline_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_asset_id_tests
        oros.assets.asset_id
        tests/assets/asset_id_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_asset_import_pipeline_tests
        oros.assets.asset_import_pipeline
        tests/assets/asset_import_pipeline_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_asset_importer_tests
        oros.assets.asset_importer
        tests/assets/asset_importer_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_asset_importer_registry_tests
        oros.assets.asset_importer_registry
        tests/assets/asset_importer_registry_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_asset_lifecycle_tests
        oros.assets.asset_lifecycle
        tests/assets/asset_lifecycle_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_asset_load_pipeline_tests
        oros.assets.asset_load_pipeline
        tests/assets/asset_load_pipeline_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_asset_record_tests
        oros.assets.asset_record
        tests/assets/asset_record_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_binary_asset_cooker_tests
        oros.assets.binary_asset_cooker
        tests/assets/binary_asset_cooker_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_binary_asset_importer_tests
        oros.assets.binary_asset_importer
        tests/assets/binary_asset_importer_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_binary_asset_pipeline_tests
        oros.assets.binary_asset_pipeline
        tests/assets/binary_asset_pipeline_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_content_hash_tests
        oros.assets.content_hash
        tests/assets/content_hash_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_cooked_asset_artifact_tests
        oros.assets.cooked_asset_artifact
        tests/assets/cooked_asset_artifact_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_cooked_asset_artifact_pipeline_tests
        oros.assets.cooked_asset_artifact_pipeline
        tests/assets/cooked_asset_artifact_pipeline_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_cooked_asset_artifact_serialization_tests
        oros.assets.cooked_asset_artifact_serialization
        tests/assets/cooked_asset_artifact_serialization_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_cooked_asset_artifact_store_tests
        oros.assets.cooked_asset_artifact_store
        tests/assets/cooked_asset_artifact_store_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_cooked_asset_cache_tests
        oros.assets.cooked_asset_cache
        tests/assets/cooked_asset_cache_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_resource_handle_tests
        oros.assets.resource_handle
        tests/assets/resource_handle_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_resource_table_tests
        oros.assets.resource_table
        tests/assets/resource_table_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_assets_run_length_compression_codec_tests
        oros.assets.run_length_compression_codec
        tests/assets/run_length_compression_codec_tests.cpp
        oros_assets
    )

    oros_add_test_target(
        oros_streaming_world_cell_key_tests
        oros.streaming.world_cell_key
        tests/streaming/world_cell_key_tests.cpp
        oros_streaming
    )

    oros_add_test_target(
        oros_streaming_world_cell_revision_id_tests
        oros.streaming.world_cell_revision_id
        tests/streaming/world_cell_revision_id_tests.cpp
        oros_streaming
    )

    oros_add_test_target(
        oros_streaming_world_cell_snapshot_tests
        oros.streaming.world_cell_snapshot
        tests/streaming/world_cell_snapshot_tests.cpp
        oros_streaming
    )

    oros_add_test_target(
        oros_streaming_world_cell_snapshot_manifest_tests
        oros.streaming.world_cell_snapshot_manifest
        tests/streaming/world_cell_snapshot_manifest_tests.cpp
        oros_streaming
    )

    oros_add_test_target(
        oros_streaming_world_cell_persistence_store_tests
        oros.streaming.world_cell_persistence_store
        tests/streaming/world_cell_persistence_store_tests.cpp
        oros_streaming
    )

    oros_add_test_target(
        oros_streaming_world_cell_io_service_tests
        oros.streaming.world_cell_io_service
        tests/streaming/world_cell_io_service_tests.cpp
        oros_streaming
    )

    target_include_directories(
        oros_streaming_world_cell_io_service_tests
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/engine/streaming/src"
    )

    oros_add_test_target(
        oros_streaming_world_cell_residency_tests
        oros.streaming.world_cell_residency
        tests/streaming/world_cell_residency_tests.cpp
        oros_streaming
    )

    oros_add_test_target(
        oros_streaming_world_cell_streaming_policy_tests
        oros.streaming.world_cell_streaming_policy
        tests/streaming/world_cell_streaming_policy_tests.cpp
        oros_streaming
    )

    oros_add_test_target(
        oros_streaming_world_cell_streaming_lifecycle_tests
        oros.streaming.world_cell_streaming_lifecycle
        tests/streaming/world_cell_streaming_lifecycle_tests.cpp
        oros_streaming
    )

    oros_add_test_target(
        oros_physical_world_world_cell_collider_set_tests
        oros.physical_world.world_cell_collider_set
        tests/physical_world/world_cell_collider_set_tests.cpp
        oros_physical_world
    )

    oros_add_test_target(
        oros_physical_world_world_cell_collider_payload_tests
        oros.physical_world.world_cell_collider_payload
        tests/physical_world/world_cell_collider_payload_tests.cpp
        oros_physical_world
    )

    oros_add_test_target(
        oros_physical_world_world_cell_collider_registry_tests
        oros.physical_world.world_cell_collider_registry
        tests/physical_world/world_cell_collider_registry_tests.cpp
        oros_physical_world
    )

    oros_add_test_target(
        oros_physical_world_world_capsule_contact_query_tests
        oros.physical_world.world_capsule_contact_query
        tests/physical_world/world_capsule_contact_query_tests.cpp
        oros_physical_world
    )

    oros_add_test_target(
        oros_physical_world_world_capsule_depenetration_tests
        oros.physical_world.world_capsule_depenetration
        tests/physical_world/world_capsule_depenetration_tests.cpp
        oros_physical_world
    )

    oros_add_test_target(
        oros_physical_world_world_capsule_motion_tests
        oros.physical_world.world_capsule_motion
        tests/physical_world/world_capsule_motion_tests.cpp
        oros_physical_world
    )

    oros_add_test_target(
        oros_physical_world_world_capsule_traversal_settings_tests
        oros.physical_world.world_capsule_traversal_settings
        tests/physical_world/world_capsule_traversal_settings_tests.cpp
        oros_physical_world
    )

    oros_add_test_target(
        oros_physical_world_world_capsule_ground_query_tests
        oros.physical_world.world_capsule_ground_query
        tests/physical_world/world_capsule_ground_query_tests.cpp
        oros_physical_world
    )

    oros_add_test_target(
        oros_physical_world_world_capsule_ground_support_probe_tests
        oros.physical_world.world_capsule_ground_support_probe
        tests/physical_world/world_capsule_ground_support_probe_tests.cpp
        oros_physical_world
    )

    oros_add_test_target(
        oros_physical_world_world_capsule_ground_snap_tests
        oros.physical_world.world_capsule_ground_snap
        tests/physical_world/world_capsule_ground_snap_tests.cpp
        oros_physical_world
    )

    oros_add_test_target(
        oros_physical_world_world_capsule_ground_motion_tests
        oros.physical_world.world_capsule_ground_motion
        tests/physical_world/world_capsule_ground_motion_tests.cpp
        oros_physical_world
    )

    oros_add_test_target(
        oros_physical_world_world_first_person_controller_tests
        oros.physical_world.world_first_person_controller
        tests/physical_world/world_first_person_controller_tests.cpp
        oros_physical_world
    )

    oros_add_test_target(
        oros_physical_world_world_capsule_step_up_tests
        oros.physical_world.world_capsule_step_up
        tests/physical_world/world_capsule_step_up_tests.cpp
        oros_physical_world
    )
    oros_add_test_target(
        oros_ai_actor_simulation_fidelity_state_tests
        oros.ai.actor_simulation_fidelity_state
        tests/ai/actor_simulation_fidelity_state_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_simulation_fidelity_registry_tests
        oros.ai.actor_simulation_fidelity_registry
        tests/ai/actor_simulation_fidelity_registry_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_simulation_focus_policy_tests
        oros.ai.actor_simulation_focus_policy
        tests/ai/actor_simulation_focus_policy_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_simulation_focus_source_registry_tests
        oros.ai.actor_simulation_focus_source_registry
        tests/ai/actor_simulation_focus_source_registry_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_simulation_focus_decision_tests
        oros.ai.actor_simulation_focus_decision
        tests/ai/actor_simulation_focus_decision_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_simulation_focus_decision_application_tests
        oros.ai.actor_simulation_focus_decision_application
        tests/ai/actor_simulation_focus_decision_application_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_simulation_position_snapshot_set_tests
        oros.ai.actor_simulation_position_snapshot_set
        tests/ai/actor_simulation_position_snapshot_set_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_simulation_focus_decision_set_tests
        oros.ai.actor_simulation_focus_decision_set
        tests/ai/actor_simulation_focus_decision_set_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_simulation_focus_decision_set_application_tests
        oros.ai.actor_simulation_focus_decision_set_application
        tests/ai/actor_simulation_focus_decision_set_application_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_schedule_window_tests
        oros.ai.actor_schedule_window
        tests/ai/actor_schedule_window_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_activity_intent_key_tests
        oros.ai.actor_activity_intent_key
        tests/ai/actor_activity_intent_key_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_faction_key_tests
        oros.ai.faction_key
        tests/ai/faction_key_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_faction_relationship_tests
        oros.ai.faction_relationship
        tests/ai/faction_relationship_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_faction_relationship_matrix_tests
        oros.ai.faction_relationship_matrix
        tests/ai/faction_relationship_matrix_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_faction_trust_tests
        oros.ai.faction_trust
        tests/ai/faction_trust_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_faction_trust_matrix_tests
        oros.ai.faction_trust_matrix
        tests/ai/faction_trust_matrix_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_faction_membership_tests
        oros.ai.actor_faction_membership
        tests/ai/actor_faction_membership_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_faction_membership_set_tests
        oros.ai.actor_faction_membership_set
        tests/ai/actor_faction_membership_set_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_faction_reputation_tests
        oros.ai.actor_faction_reputation
        tests/ai/actor_faction_reputation_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_faction_reputation_set_tests
        oros.ai.actor_faction_reputation_set
        tests/ai/actor_faction_reputation_set_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_navigation_node_id_tests
        oros.ai.navigation_node_id
        tests/ai/navigation_node_id_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_navigation_portal_id_tests
        oros.ai.navigation_portal_id
        tests/ai/navigation_portal_id_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_scheduled_activity_tests
        oros.ai.actor_scheduled_activity
        tests/ai/actor_scheduled_activity_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_schedule_tests
        oros.ai.actor_schedule
        tests/ai/actor_schedule_tests.cpp
        oros_ai
    )

    oros_add_test_target(
        oros_ai_actor_schedule_execution_state_tests
        oros.ai.actor_schedule_execution_state
        tests/ai/actor_schedule_execution_state_tests.cpp
        oros_ai
    )
endif()