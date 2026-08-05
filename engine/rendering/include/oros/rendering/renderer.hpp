#pragma once

#include "oros/foundation/result.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace oros::rendering
{
    struct ClearColor final
    {
        float red{0.015F};
        float green{0.025F};
        float blue{0.060F};
        float alpha{1.0F};
    };

    struct RendererConfig final
    {
        std::uint32_t frame_buffer_count{3};

        bool enable_debug_layer{false};
        bool enable_gpu_validation{false};
        bool prefer_high_performance_adapter{true};

        bool vertical_synchronization{true};
        bool allow_tearing{true};
    };

    struct RendererInfo final
    {
        std::string adapter_name{};

        std::uint64_t dedicated_video_memory_bytes{};
        std::uint64_t shared_system_memory_bytes{};

        std::uint32_t frame_buffer_count{};

        bool debug_layer_enabled{};
        bool gpu_validation_enabled{};
        bool tearing_supported{};
    };

    class Renderer final
    {
    public:
        [[nodiscard]] static foundation::Result<Renderer>
        create(
            void* native_window_handle,
            RendererConfig config = {});

        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        Renderer(Renderer&& other) noexcept;
        Renderer& operator=(Renderer&& other) noexcept;

        [[nodiscard]] foundation::Status
        draw_frame(
            const ClearColor& clear_color);

        [[nodiscard]] foundation::Status
        resize(
            std::uint32_t width,
            std::uint32_t height);

        [[nodiscard]] foundation::Status
        wait_idle();

        [[nodiscard]] bool
        is_ready() const noexcept;

        [[nodiscard]] const RendererInfo&
        info() const noexcept;

    private:
        struct Implementation;

        explicit Renderer(
            std::unique_ptr<Implementation>
                implementation) noexcept;

        std::unique_ptr<Implementation> implementation_;
    };
}