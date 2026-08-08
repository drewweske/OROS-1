#include "physical_world_demo.hpp"

#include "oros/foundation/clock.hpp"
#include "oros/foundation/log.hpp"
#include "oros/physical_world/world_first_person_controller.hpp"
#include "oros/physics/physics_vector.hpp"
#include "oros/platform/window.hpp"
#include "oros/rendering/renderer.hpp"
#include "oros/runtime/engine_runtime.hpp"
#include "oros/world/world.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace oros
{
    inline constexpr std::string_view engine_name{
        "OROS 1"
    };

    inline constexpr std::uint64_t
        bootstrap_world_namespace{
            0x4F524F53ULL
        };

    struct Version final
    {
        int major;
        int minor;
        int patch;
    };

    inline constexpr Version engine_version{
        OROS_VERSION_MAJOR,
        OROS_VERSION_MINOR,
        OROS_VERSION_PATCH
    };
}

int main()
{
    using namespace oros::foundation;
    using namespace oros::platform;
    using namespace oros::rendering;
    using namespace oros::runtime;
    using namespace oros::world;

    Stopwatch startup_timer{};

    const Status logging_status =
        initialize_logging();

    if (!logging_status.has_value())
    {
        const Error& error =
            logging_status.error();

        std::cerr
            << "OROS logging initialization failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        return 1;
    }

    write_log(
        LogLevel::info,
        "bootstrap",
        "OROS logging initialized.");

    std::cout
        << oros::engine_name
        << " Engine Bootstrap\n";

    std::cout
        << "Version "
        << oros::engine_version.major
        << '.'
        << oros::engine_version.minor
        << '.'
        << oros::engine_version.patch
        << '\n';

    std::cout
        << "OROS-000 Genesis: PASS\n";

    std::cout
        << "OROS-001 Foundation: PASS\n";

    std::cout
        << "OROS-002 Platform: PASS\n";

    Result<World> world_result =
        World::create(
            oros::bootstrap_world_namespace);

    if (!world_result.has_value())
    {
        const Error& error =
            world_result.error();

        write_log(
            LogLevel::critical,
            "world",
            "World creation failed: [" +
                std::string{to_string(error.code)} +
                "] " +
                error.message);

        std::cerr
            << "OROS World creation failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        shutdown_logging();
        return 1;
    }

    World world{
        std::move(world_result.value())
    };

    Result<EntityId> bootstrap_entity_result =
        world.create_entity();

    if (!bootstrap_entity_result.has_value())
    {
        const Error& error =
            bootstrap_entity_result.error();

        write_log(
            LogLevel::critical,
            "world",
            "Bootstrap entity creation failed: [" +
                std::string{to_string(error.code)} +
                "] " +
                error.message);

        std::cerr
            << "OROS bootstrap entity creation failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        shutdown_logging();
        return 1;
    }

    const EntityId bootstrap_entity =
        bootstrap_entity_result.value();

    Result<WorldPosition> bootstrap_position_result =
        WorldPosition::create(
            WorldCell{
                1'000'000,
                0,
                -1'000'000
            },
            LocalPosition{
                128.0,
                64.0,
                -256.0
            });

    if (!bootstrap_position_result.has_value())
    {
        const Error& error =
            bootstrap_position_result.error();

        write_log(
            LogLevel::critical,
            "world",
            "Bootstrap position creation failed: [" +
                std::string{to_string(error.code)} +
                "] " +
                error.message);

        std::cerr
            << "OROS bootstrap position creation failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        shutdown_logging();
        return 1;
    }

    const Status bootstrap_position_status =
        world.add_position(
            bootstrap_entity,
            std::move(
                bootstrap_position_result.value()));

    if (!bootstrap_position_status.has_value())
    {
        const Error& error =
            bootstrap_position_status.error();

        write_log(
            LogLevel::critical,
            "world",
            "Bootstrap position attachment failed: [" +
                std::string{to_string(error.code)} +
                "] " +
                error.message);

        std::cerr
            << "OROS bootstrap position attachment failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        shutdown_logging();
        return 1;
    }

    const WorldPosition* bootstrap_position =
        world.find_position(
            bootstrap_entity);

    if (bootstrap_position == nullptr)
    {
        write_log(
            LogLevel::critical,
            "world",
            "The bootstrap entity lost its position "
            "immediately after attachment.");

        std::cerr
            << "OROS bootstrap entity position could not "
            << "be found after attachment.\n";

        shutdown_logging();
        return 1;
    }

    const WorldCell bootstrap_world_cell =
        bootstrap_position->cell();

    const LocalPosition bootstrap_local_position =
        bootstrap_position->local();

    write_log(
        LogLevel::info,
        "world",
        "World namespace " +
            std::to_string(
                world.world_namespace()) +
            " initialized with bootstrap entity " +
            oros::world::to_string(
                bootstrap_entity) +
            ".");

    write_log(
        LogLevel::info,
        "world",
        "Bootstrap entity position cell: " +
            std::to_string(
                bootstrap_world_cell.x) +
            ", " +
            std::to_string(
                bootstrap_world_cell.y) +
            ", " +
            std::to_string(
                bootstrap_world_cell.z) +
            ".");

    Result<EntityId> player_entity_result =
        world.create_entity();

    if (!player_entity_result.has_value())
    {
        const Error& error =
            player_entity_result.error();

        write_log(
            LogLevel::critical,
            "physical_world",
            "Live player entity creation failed: [" +
                std::string{to_string(error.code)} +
                "] " +
                error.message);

        std::cerr
            << "OROS live player entity creation failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        shutdown_logging();
        return 1;
    }

    const EntityId player_entity =
        player_entity_result.value();

    Result<EntityId> floor_entity_result =
        world.create_entity();

    if (!floor_entity_result.has_value())
    {
        const Error& error =
            floor_entity_result.error();

        write_log(
            LogLevel::critical,
            "physical_world",
            "Live floor entity creation failed: [" +
                std::string{to_string(error.code)} +
                "] " +
                error.message);

        std::cerr
            << "OROS live floor entity creation failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        shutdown_logging();
        return 1;
    }

    const EntityId floor_entity =
        floor_entity_result.value();

    Result<WorldPosition> player_position_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                0.0,
                0.0,
                0.0
            });

    if (!player_position_result.has_value())
    {
        const Error& error =
            player_position_result.error();

        write_log(
            LogLevel::critical,
            "physical_world",
            "Live player position creation failed: [" +
                std::string{to_string(error.code)} +
                "] " +
                error.message);

        std::cerr
            << "OROS live player position creation failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        shutdown_logging();
        return 1;
    }

    const Status player_position_status =
        world.add_position(
            player_entity,
            std::move(
                player_position_result.value()));

    if (!player_position_status.has_value())
    {
        const Error& error =
            player_position_status.error();

        write_log(
            LogLevel::critical,
            "physical_world",
            "Live player position attachment failed: [" +
                std::string{to_string(error.code)} +
                "] " +
                error.message);

        std::cerr
            << "OROS live player position attachment failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        shutdown_logging();
        return 1;
    }

    Result<oros::bootstrap::PhysicalWorldDemo>
        physical_world_demo_result =
            oros::bootstrap::
                create_physical_world_demo(
                    world.world_namespace(),
                    player_entity,
                    floor_entity);

    if (!physical_world_demo_result.has_value())
    {
        const Error& error =
            physical_world_demo_result.error();

        write_log(
            LogLevel::critical,
            "physical_world",
            "Live physical-world composition failed: [" +
                std::string{to_string(error.code)} +
                "] " +
                error.message);

        std::cerr
            << "OROS live physical-world composition failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        shutdown_logging();
        return 1;
    }

    oros::bootstrap::PhysicalWorldDemo
        physical_world_demo{
            std::move(
                physical_world_demo_result.value())
        };

    const WorldPosition* initial_player_position =
        world.find_position(
            player_entity);

    if (initial_player_position == nullptr)
    {
        write_log(
            LogLevel::critical,
            "physical_world",
            "The live player lost its authoritative "
            "WorldPosition immediately after attachment.");

        std::cerr
            << "OROS live player position could not be "
            << "found after attachment.\n";

        shutdown_logging();
        return 1;
    }

    if (!world.contains(player_entity) ||
        !world.contains(floor_entity) ||
        physical_world_demo.
                player_collider.
                owner !=
            player_entity ||
        physical_world_demo.
                floor_collider.
                owner !=
            floor_entity ||
        physical_world_demo.
                registry.
                active_cell_count() != 1U ||
        physical_world_demo.
                registry.
                collider_count() != 1U ||
        !physical_world_demo.
            registry.
            contains(
                physical_world_demo.
                    floor_collider))
    {
        write_log(
            LogLevel::critical,
            "physical_world",
            "Live physical-world identity or residency "
            "contract was not preserved.");

        std::cerr
            << "OROS live physical-world identity or "
            << "residency contract failed.\n";

        shutdown_logging();
        return 1;
    }

    const WorldCell initial_player_world_cell =
        initial_player_position->cell();

    const LocalPosition initial_player_local_position =
        initial_player_position->local();

    write_log(
        LogLevel::info,
        "physical_world",
        "Live player " +
            oros::world::to_string(
                player_entity) +
            " initialized with world-owned position.");

    write_log(
        LogLevel::info,
        "physical_world",
        "Resident floor " +
            oros::world::to_string(
                floor_entity) +
            " activated for the live controller.");

    WindowConfig window_config{};
    window_config.title =
        "OROS 1 - OROS-005 World";
    window_config.client_width = 1280;
    window_config.client_height = 720;
    window_config.resizable = true;
    window_config.start_visible = true;

    Result<Window> window_result =
        Window::create(
            std::move(window_config));

    if (!window_result.has_value())
    {
        const Error& error =
            window_result.error();

        write_log(
            LogLevel::critical,
            "platform",
            "Native window creation failed: [" +
                std::string{to_string(error.code)} +
                "] " +
                error.message);

        std::cerr
            << "OROS native window creation failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        shutdown_logging();
        return 1;
    }

    int exit_code = 0;

    {
        Window window{
            std::move(window_result.value())
        };

        WindowExtent current_extent =
            window.client_extent();

        write_log(
            LogLevel::info,
            "platform",
            std::string{
                "Native window created with client extent "
            } +
                std::to_string(current_extent.width) +
                "x" +
                std::to_string(current_extent.height) +
                ".");

        RendererConfig renderer_config{};
        renderer_config.frame_buffer_count = 3U;
        renderer_config.enable_debug_layer = false;
        renderer_config.enable_gpu_validation = false;
        renderer_config.prefer_high_performance_adapter =
            true;
        renderer_config.vertical_synchronization = true;
        renderer_config.allow_tearing = true;

        Result<Renderer> renderer_result =
            Renderer::create(
                window.native_handle(),
                renderer_config);

        if (!renderer_result.has_value())
        {
            const Error& error =
                renderer_result.error();

            write_log(
                LogLevel::critical,
                "rendering",
                "Direct3D 12 renderer creation failed: [" +
                    std::string{
                        to_string(error.code)} +
                    "] " +
                    error.message);

            std::cerr
                << "OROS renderer creation failed: ["
                << to_string(error.code)
                << "] "
                << error.message
                << '\n';

            exit_code = 1;
        }
        else
        {
            {
                Renderer renderer{
                    std::move(renderer_result.value())
                };

                const RendererInfo& renderer_info =
                    renderer.info();

                const std::uint64_t
                    dedicated_memory_mib =
                        renderer_info.
                            dedicated_video_memory_bytes /
                        (1024ULL * 1024ULL);

                write_log(
                    LogLevel::info,
                    "rendering",
                    "Direct3D 12 renderer initialized on " +
                        renderer_info.adapter_name +
                        ".");

                write_log(
                    LogLevel::info,
                    "rendering",
                    "Dedicated video memory: " +
                        std::to_string(
                            dedicated_memory_mib) +
                        " MiB.");

                EngineRuntimeConfig runtime_config{};

                Result<EngineRuntime> runtime_result =
                    EngineRuntime::create(
                        runtime_config);

                if (!runtime_result.has_value())
                {
                    const Error& error =
                        runtime_result.error();

                    write_log(
                        LogLevel::critical,
                        "runtime",
                        "Engine runtime creation failed: [" +
                            std::string{
                                to_string(error.code)} +
                            "] " +
                            error.message);

                    std::cerr
                        << "OROS runtime creation failed: ["
                        << to_string(error.code)
                        << "] "
                        << error.message
                        << '\n';

                    exit_code = 1;
                }
                else
                {
                    EngineRuntime runtime{
                        std::move(runtime_result.value())
                    };

                    const Status runtime_start_status =
                        runtime.start(
                            monotonic_now());

                    if (!runtime_start_status.has_value())
                    {
                        const Error& error =
                            runtime_start_status.error();

                        write_log(
                            LogLevel::critical,
                            "runtime",
                            "Engine runtime startup failed: [" +
                                std::string{
                                    to_string(
                                        error.code)} +
                                "] " +
                                error.message);

                        std::cerr
                            << "OROS runtime startup failed: ["
                            << to_string(error.code)
                            << "] "
                            << error.message
                            << '\n';

                        exit_code = 1;
                    }
                    else
                    {
                        write_log(
                            LogLevel::info,
                            "runtime",
                            "Engine runtime entered the " +
                                std::string{
                                    to_string(
                                        runtime.state())} +
                                " state.");

                        std::cout
                            << "OROS-003 First Light: PASS\n";

                        std::cout
                            << "OROS-004 Runtime: ACTIVE\n";

                        std::cout
                            << "OROS-005 World: ACTIVE\n";

                        std::cout
                            << "World namespace: "
                            << world.world_namespace()
                            << '\n';

                        std::cout
                            << "World entities: "
                            << world.entity_count()
                            << '\n';

                        std::cout
                            << "World positions: "
                            << world.position_count()
                            << '\n';

                        std::cout
                            << "Bootstrap entity: "
                            << oros::world::to_string(
                                bootstrap_entity)
                            << '\n';

                        std::cout
                            << "Bootstrap world cell: "
                            << bootstrap_world_cell.x
                            << ", "
                            << bootstrap_world_cell.y
                            << ", "
                            << bootstrap_world_cell.z
                            << '\n';

                        std::cout
                            << "Bootstrap local position: "
                            << bootstrap_local_position.x
                            << ", "
                            << bootstrap_local_position.y
                            << ", "
                            << bootstrap_local_position.z
                            << " meters\n";

                        std::cout
                            << "Live player entity: "
                            << oros::world::to_string(
                                player_entity)
                            << '\n';

                        std::cout
                            << "Live floor entity: "
                            << oros::world::to_string(
                                floor_entity)
                            << '\n';

                        std::cout
                            << "Live player world cell: "
                            << initial_player_world_cell.x
                            << ", "
                            << initial_player_world_cell.y
                            << ", "
                            << initial_player_world_cell.z
                            << '\n';

                        std::cout
                            << "Live player local position: "
                            << initial_player_local_position.x
                            << ", "
                            << initial_player_local_position.y
                            << ", "
                            << initial_player_local_position.z
                            << " meters\n";

                        std::cout
                            << "Resident collision cells: "
                            << physical_world_demo.
                                registry.
                                active_cell_count()
                            << '\n';

                        std::cout
                            << "Resident colliders: "
                            << physical_world_demo.
                                registry.
                                collider_count()
                            << '\n';

                        std::cout
                            << "Renderer: Direct3D 12\n";

                        std::cout
                            << "Adapter: "
                            << renderer_info.adapter_name
                            << '\n';

                        std::cout
                            << "Dedicated video memory: "
                            << dedicated_memory_mib
                            << " MiB\n";

                        std::cout
                            << "Frame buffers: "
                            << renderer_info.
                                frame_buffer_count
                            << '\n';

                        std::cout
                            << "Native window: "
                            << current_extent.width
                            << 'x'
                            << current_extent.height
                            << '\n';

                        std::cout
                            << "Fixed simulation step: "
                            << runtime.config().
                                fixed_step.
                                    simulation_step.count()
                            << " nanoseconds\n";

                        std::cout
                            << "Maximum updates per frame: "
                            << runtime.config().
                                fixed_step.
                                    maximum_updates_per_frame
                            << '\n';

                        std::cout
                            << "Hold W/A/S/D to move the live "
                            << "grounded controller.\n";

                        std::cout
                            << "Close the window or press "
                            << "Escape to exit.\n";

                        const auto startup_microseconds =
                            std::chrono::duration_cast<
                                std::chrono::microseconds>(
                                    startup_timer.elapsed())
                                .count();

                        write_log(
                            LogLevel::info,
                            "bootstrap",
                            "Bootstrap completed in " +
                                std::to_string(
                                    startup_microseconds) +
                                " microseconds.");

                        const ClearColor first_light_color{
                            0.015F,
                            0.075F,
                            0.140F,
                            1.0F
                        };

                        const auto request_runtime_stop =
                            [&runtime, &exit_code](
                                const std::string_view reason)
                            {
                                const Status request_status =
                                    runtime.request_stop();

                                if (!request_status.has_value())
                                {
                                    const Error& error =
                                        request_status.error();

                                    write_log(
                                        LogLevel::critical,
                                        "runtime",
                                        "Runtime stop request "
                                        "failed: [" +
                                            std::string{
                                                to_string(
                                                    error.code)} +
                                            "] " +
                                            error.message);

                                    std::cerr
                                        << "OROS runtime stop "
                                        << "request failed: ["
                                        << to_string(
                                            error.code)
                                        << "] "
                                        << error.message
                                        << '\n';

                                    exit_code = 1;
                                    return false;
                                }

                                write_log(
                                    LogLevel::info,
                                    "runtime",
                                    "Runtime stop requested: " +
                                        std::string{reason} +
                                        ".");

                                return true;
                            };

                        while (runtime.is_running())
                        {
                            window.pump_events();

                            if (window.should_close())
                            {
                                if (!request_runtime_stop(
                                        "native window closed"))
                                {
                                    break;
                                }

                                continue;
                            }

                            const InputState& input =
                                window.input_state();

                            if (input.key_pressed(Key::escape))
                            {
                                window.request_close();

                                if (!request_runtime_stop(
                                        "Escape key pressed"))
                                {
                                    break;
                                }

                                continue;
                            }

                            double movement_x = 0.0;
                            double movement_z = 0.0;

                            if (input.key_down(Key::d))
                            {
                                movement_x += 1.0;
                            }

                            if (input.key_down(Key::a))
                            {
                                movement_x -= 1.0;
                            }

                            if (input.key_down(Key::w))
                            {
                                movement_z += 1.0;
                            }

                            if (input.key_down(Key::s))
                            {
                                movement_z -= 1.0;
                            }

                            const oros::physical_world::
                                WorldFirstPersonControllerCommand
                                movement_command{
                                    oros::physics::
                                        PhysicsVector3{
                                            movement_x,
                                            0.0,
                                            movement_z
                                        }
                                };

                            Result<RuntimeFrame> frame_result =
                                runtime.begin_frame(
                                    monotonic_now());

                            if (!frame_result.has_value())
                            {
                                const Error& error =
                                    frame_result.error();

                                write_log(
                                    LogLevel::critical,
                                    "runtime",
                                    "Runtime frame failed: [" +
                                        std::string{
                                            to_string(
                                                error.code)} +
                                        "] " +
                                        error.message);

                                std::cerr
                                    << "OROS runtime frame "
                                    << "failed: ["
                                    << to_string(error.code)
                                    << "] "
                                    << error.message
                                    << '\n';

                                exit_code = 1;

                                if (!request_runtime_stop(
                                        "runtime frame failure"))
                                {
                                    break;
                                }

                                continue;
                            }

                            const FrameSchedule&
                                frame_schedule =
                                    frame_result.
                                        value().
                                        schedule;

                            const Nanoseconds
                                simulation_step =
                                    runtime.
                                        config().
                                        fixed_step.
                                        simulation_step;

                            bool simulation_frame_failed =
                                false;

                            bool simulation_stop_failed =
                                false;

                            for (
                                std::uint32_t
                                    simulation_update_index =
                                        0U;
                                simulation_update_index <
                                    frame_schedule.
                                        simulation_update_count;
                                ++simulation_update_index)
                            {
                                WorldPosition*
                                    player_position =
                                        world.find_position(
                                            player_entity);

                                if (player_position ==
                                    nullptr)
                                {
                                    write_log(
                                        LogLevel::critical,
                                        "physical_world",
                                        "Live player lost its "
                                        "authoritative "
                                        "WorldPosition during "
                                        "simulation.");

                                    std::cerr
                                        << "OROS live player "
                                        << "WorldPosition was "
                                        << "lost during "
                                        << "simulation.\n";

                                    exit_code = 1;

                                    simulation_frame_failed =
                                        true;

                                    if (!request_runtime_stop(
                                            "live player "
                                            "position loss"))
                                    {
                                        simulation_stop_failed =
                                            true;
                                    }

                                    break;
                                }

                                Result<WorldPosition>
                                    controller_step_result =
                                        oros::
                                            physical_world::
                                            step_world_first_person_controller(
                                                physical_world_demo.
                                                    registry,
                                                world.
                                                    world_namespace(),
                                                physical_world_demo.
                                                    player_collider,
                                                physical_world_demo.
                                                    player_capsule,
                                                *player_position,
                                                movement_command,
                                                physical_world_demo.
                                                    traversal_settings,
                                                physical_world_demo.
                                                    controller_settings,
                                                simulation_step);

                                if (!controller_step_result.
                                        has_value())
                                {
                                    const Error& error =
                                        controller_step_result.
                                            error();

                                    write_log(
                                        LogLevel::critical,
                                        "physical_world",
                                        "Live controller fixed "
                                        "update failed: [" +
                                            std::string{
                                                to_string(
                                                    error.code)} +
                                            "] " +
                                            error.message);

                                    std::cerr
                                        << "OROS live controller "
                                        << "fixed update failed: ["
                                        << to_string(
                                            error.code)
                                        << "] "
                                        << error.message
                                        << '\n';

                                    exit_code = 1;

                                    simulation_frame_failed =
                                        true;

                                    if (!request_runtime_stop(
                                            "live controller "
                                            "simulation failure"))
                                    {
                                        simulation_stop_failed =
                                            true;
                                    }

                                    break;
                                }

                                *player_position =
                                    std::move(
                                        controller_step_result.
                                            value());

                                if (!player_position->
                                        is_normalized())
                                {
                                    write_log(
                                        LogLevel::critical,
                                        "physical_world",
                                        "Live controller produced "
                                        "a non-normalized "
                                        "WorldPosition.");

                                    std::cerr
                                        << "OROS live controller "
                                        << "produced a "
                                        << "non-normalized "
                                        << "WorldPosition.\n";

                                    exit_code = 1;

                                    simulation_frame_failed =
                                        true;

                                    if (!request_runtime_stop(
                                            "live player "
                                            "normalization failure"))
                                    {
                                        simulation_stop_failed =
                                            true;
                                    }

                                    break;
                                }
                            }

                            if (simulation_stop_failed)
                            {
                                break;
                            }

                            if (simulation_frame_failed)
                            {
                                continue;
                            }

                            const WindowExtent new_extent =
                                window.client_extent();

                            if (new_extent.width !=
                                    current_extent.width ||
                                new_extent.height !=
                                    current_extent.height)
                            {
                                const Status resize_status =
                                    renderer.resize(
                                        new_extent.width,
                                        new_extent.height);

                                if (!resize_status.has_value())
                                {
                                    const Error& error =
                                        resize_status.error();

                                    write_log(
                                        LogLevel::critical,
                                        "rendering",
                                        "Renderer resize "
                                        "failed: [" +
                                            std::string{
                                                to_string(
                                                    error.code)} +
                                            "] " +
                                            error.message);

                                    std::cerr
                                        << "OROS renderer "
                                        << "resize failed: ["
                                        << to_string(
                                            error.code)
                                        << "] "
                                        << error.message
                                        << '\n';

                                    exit_code = 1;

                                    if (!request_runtime_stop(
                                            "renderer resize "
                                            "failure"))
                                    {
                                        break;
                                    }

                                    continue;
                                }

                                current_extent = new_extent;
                            }

                            if (current_extent.width == 0U ||
                                current_extent.height == 0U)
                            {
                                std::this_thread::sleep_for(
                                    std::chrono::
                                        milliseconds{16});

                                continue;
                            }

                            const Status frame_status =
                                renderer.draw_frame(
                                    first_light_color);

                            if (!frame_status.has_value())
                            {
                                const Error& error =
                                    frame_status.error();

                                write_log(
                                    LogLevel::critical,
                                    "rendering",
                                    "Frame rendering failed: [" +
                                        std::string{
                                            to_string(
                                                error.code)} +
                                        "] " +
                                        error.message);

                                std::cerr
                                    << "OROS frame rendering "
                                    << "failed: ["
                                    << to_string(error.code)
                                    << "] "
                                    << error.message
                                    << '\n';

                                exit_code = 1;

                                if (!request_runtime_stop(
                                        "frame rendering "
                                        "failure"))
                                {
                                    break;
                                }
                            }
                        }

                        const Status idle_status =
                            renderer.wait_idle();

                        if (!idle_status.has_value())
                        {
                            const Error& error =
                                idle_status.error();

                            write_log(
                                LogLevel::error,
                                "rendering",
                                "Final GPU wait failed: [" +
                                    std::string{
                                        to_string(
                                            error.code)} +
                                    "] " +
                                    error.message);

                            std::cerr
                                << "OROS final GPU wait "
                                << "failed: ["
                                << to_string(error.code)
                                << "] "
                                << error.message
                                << '\n';

                            exit_code = 1;
                        }

                        if (runtime.is_stop_requested())
                        {
                            const Status runtime_stop_status =
                                runtime.stop();

                            if (!runtime_stop_status.
                                    has_value())
                            {
                                const Error& error =
                                    runtime_stop_status.error();

                                write_log(
                                    LogLevel::critical,
                                    "runtime",
                                    "Engine runtime shutdown "
                                    "failed: [" +
                                        std::string{
                                            to_string(
                                                error.code)} +
                                        "] " +
                                        error.message);

                                std::cerr
                                    << "OROS runtime shutdown "
                                    << "failed: ["
                                    << to_string(error.code)
                                    << "] "
                                    << error.message
                                    << '\n';

                                exit_code = 1;
                            }
                        }
                        else if (runtime.is_running())
                        {
                            write_log(
                                LogLevel::critical,
                                "runtime",
                                "The engine loop ended without "
                                "a legal runtime stop request.");

                            std::cerr
                                << "OROS engine loop ended "
                                << "without a legal runtime "
                                << "stop request.\n";

                            exit_code = 1;
                        }

                        if (runtime.is_stopped())
                        {
                            const std::uint64_t
                                completed_frames =
                                    runtime.scheduler().
                                        completed_frame_count();

                            const std::uint64_t
                                completed_ticks =
                                    runtime.scheduler().
                                        completed_simulation_tick_count();

                            const auto dropped_microseconds =
                                std::chrono::duration_cast<
                                    std::chrono::microseconds>(
                                        runtime.scheduler().
                                            dropped_time())
                                    .count();

                            write_log(
                                LogLevel::info,
                                "runtime",
                                "Engine runtime stopped after " +
                                    std::to_string(
                                        completed_frames) +
                                    " frames and " +
                                    std::to_string(
                                        completed_ticks) +
                                    " simulation ticks.");

                            write_log(
                                LogLevel::info,
                                "runtime",
                                "Runtime dropped " +
                                    std::to_string(
                                        dropped_microseconds) +
                                    " microseconds.");

                            std::cout
                                << "OROS-004 Runtime: STOPPED\n";

                            std::cout
                                << "Completed runtime frames: "
                                << completed_frames
                                << '\n';

                            std::cout
                                << "Completed simulation ticks: "
                                << completed_ticks
                                << '\n';

                            std::cout
                                << "OROS-005 World: "
                                << world.entity_count()
                                << " entity, "
                                << world.position_count()
                                << " position\n";

                            const WorldPosition*
                                final_player_position =
                                    world.find_position(
                                        player_entity);

                            if (final_player_position ==
                                nullptr)
                            {
                                write_log(
                                    LogLevel::critical,
                                    "physical_world",
                                    "Live player position was "
                                    "missing during final "
                                    "diagnostics.");

                                std::cerr
                                    << "OROS live player final "
                                    << "position is missing.\n";

                                exit_code = 1;
                            }
                            else
                            {
                                const WorldCell&
                                    final_player_cell =
                                        final_player_position->
                                            cell();

                                const LocalPosition&
                                    final_player_local =
                                        final_player_position->
                                            local();

                                std::cout
                                    << "Live player final world "
                                    << "cell: "
                                    << final_player_cell.x
                                    << ", "
                                    << final_player_cell.y
                                    << ", "
                                    << final_player_cell.z
                                    << '\n';

                                std::cout
                                    << "Live player final local "
                                    << "position: "
                                    << final_player_local.x
                                    << ", "
                                    << final_player_local.y
                                    << ", "
                                    << final_player_local.z
                                    << " meters\n";

                                write_log(
                                    LogLevel::info,
                                    "physical_world",
                                    "Live player final local "
                                    "position: " +
                                        std::to_string(
                                            final_player_local.x) +
                                        ", " +
                                        std::to_string(
                                            final_player_local.y) +
                                        ", " +
                                        std::to_string(
                                            final_player_local.z) +
                                        " meters.");
                            }
                        }
                    }
                }
            }

            write_log(
                LogLevel::info,
                "rendering",
                "Direct3D 12 renderer destroyed cleanly.");
        }
    }

    write_log(
        LogLevel::info,
        "platform",
        "Native window destroyed cleanly.");

    write_log(
        LogLevel::info,
        "world",
        "World shutting down cleanly with " +
            std::to_string(
                world.entity_count()) +
            " entity and " +
            std::to_string(
                world.position_count()) +
            " position.");

    shutdown_logging();

    return exit_code;
}