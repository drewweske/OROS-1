#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "oros/rendering/renderer.hpp"

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace oros::rendering
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        [[nodiscard]] std::string
        hresult_text(const HRESULT result)
        {
            std::ostringstream stream{};

            stream
                << "HRESULT 0x"
                << std::uppercase
                << std::hex
                << std::setfill('0')
                << std::setw(8)
                << static_cast<std::uint32_t>(result);

            return stream.str();
        }

        [[nodiscard]] std::string
        failure_message(
            const std::string_view operation,
            const HRESULT result)
        {
            return std::string{operation} +
                " failed with " +
                hresult_text(result) +
                ".";
        }

        [[nodiscard]] std::string
        wide_to_utf8(const std::wstring_view text)
        {
            if (text.empty())
            {
                return {};
            }

            const int source_length =
                static_cast<int>(text.size());

            const int required_length =
                WideCharToMultiByte(
                    CP_UTF8,
                    WC_ERR_INVALID_CHARS,
                    text.data(),
                    source_length,
                    nullptr,
                    0,
                    nullptr,
                    nullptr);

            if (required_length <= 0)
            {
                return "Unknown adapter";
            }

            std::string converted(
                static_cast<std::size_t>(
                    required_length),
                '\0');

            const int written_length =
                WideCharToMultiByte(
                    CP_UTF8,
                    WC_ERR_INVALID_CHARS,
                    text.data(),
                    source_length,
                    converted.data(),
                    required_length,
                    nullptr,
                    nullptr);

            if (written_length != required_length)
            {
                return "Unknown adapter";
            }

            return converted;
        }

        [[nodiscard]] UINT
        swap_chain_flags(
            const RendererConfig& config,
            const bool tearing_supported) noexcept
        {
            if (config.allow_tearing &&
                tearing_supported)
            {
                return static_cast<UINT>(
                    DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
            }

            return 0U;
        }

        [[nodiscard]] UINT
        present_flags(
            const RendererConfig& config,
            const bool tearing_supported) noexcept
        {
            if (!config.vertical_synchronization &&
                config.allow_tearing &&
                tearing_supported)
            {
                return DXGI_PRESENT_ALLOW_TEARING;
            }

            return 0U;
        }
    }

    struct Renderer::Implementation final
    {
        HWND window_handle{};

        RendererConfig config{};
        RendererInfo renderer_info{};

        ComPtr<IDXGIFactory6> factory{};
        ComPtr<IDXGIAdapter1> adapter{};

        ComPtr<ID3D12Device> device{};
        ComPtr<ID3D12CommandQueue> command_queue{};

        ComPtr<IDXGISwapChain4> swap_chain{};

        ComPtr<ID3D12DescriptorHeap>
            render_target_view_heap{};

        std::vector<ComPtr<ID3D12Resource>>
            render_targets{};

        std::vector<ComPtr<ID3D12CommandAllocator>>
            command_allocators{};

        ComPtr<ID3D12GraphicsCommandList>
            command_list{};

        ComPtr<ID3D12Fence> fence{};

        HANDLE fence_event{};

        std::vector<std::uint64_t>
            frame_fence_values{};

        std::uint64_t next_fence_value{1};

        UINT render_target_view_descriptor_size{};
        UINT frame_index{};

        std::uint32_t width{};
        std::uint32_t height{};

        bool ready{};

        ~Implementation()
        {
            shutdown();
        }

        [[nodiscard]] foundation::Status
        initialize()
        {
            if (window_handle == nullptr ||
                !IsWindow(window_handle))
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Renderer requires a valid native "
                    "Windows window handle.");
            }

            if (config.frame_buffer_count < 2U ||
                config.frame_buffer_count > 8U)
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Renderer frame_buffer_count must "
                    "be between 2 and 8.");
            }

            if (config.enable_gpu_validation &&
                !config.enable_debug_layer)
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "GPU validation requires the "
                    "Direct3D 12 debug layer.");
            }

            RECT client_rectangle{};

            if (!GetClientRect(
                    window_handle,
                    &client_rectangle))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "GetClientRect failed while "
                    "initializing the renderer.");
            }

            const LONG client_width =
                client_rectangle.right -
                client_rectangle.left;

            const LONG client_height =
                client_rectangle.bottom -
                client_rectangle.top;

            width = static_cast<std::uint32_t>(
                std::max(client_width, 1L));

            height = static_cast<std::uint32_t>(
                std::max(client_height, 1L));

            foundation::Status status =
                initialize_debug_layer();

            if (!status.has_value())
            {
                return status;
            }

            status = create_factory();

            if (!status.has_value())
            {
                return status;
            }

            detect_tearing_support();

            status = select_adapter();

            if (!status.has_value())
            {
                return status;
            }

            status = create_device();

            if (!status.has_value())
            {
                return status;
            }

            status = create_command_queue();

            if (!status.has_value())
            {
                return status;
            }

            status = create_swap_chain();

            if (!status.has_value())
            {
                return status;
            }

            status = create_render_target_view_heap();

            if (!status.has_value())
            {
                return status;
            }

            status = create_command_allocators();

            if (!status.has_value())
            {
                return status;
            }

            status = create_command_list();

            if (!status.has_value())
            {
                return status;
            }

            status = create_render_targets();

            if (!status.has_value())
            {
                return status;
            }

            status = create_synchronization_objects();

            if (!status.has_value())
            {
                return status;
            }

            renderer_info.frame_buffer_count =
                config.frame_buffer_count;

            renderer_info.debug_layer_enabled =
                config.enable_debug_layer;

            renderer_info.gpu_validation_enabled =
                config.enable_gpu_validation;

            ready = true;

            return {};
        }

        [[nodiscard]] foundation::Status
        initialize_debug_layer()
        {
            if (!config.enable_debug_layer)
            {
                return {};
            }

            ComPtr<ID3D12Debug> debug_interface{};

            const HRESULT debug_result =
                D3D12GetDebugInterface(
                    IID_PPV_ARGS(
                        debug_interface.GetAddressOf()));

            if (FAILED(debug_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        unsupported_operation,
                    failure_message(
                        "D3D12GetDebugInterface",
                        debug_result) +
                        " Install the Windows Graphics "
                        "Tools optional feature.");
            }

            debug_interface->EnableDebugLayer();

            if (!config.enable_gpu_validation)
            {
                return {};
            }

            ComPtr<ID3D12Debug1>
                advanced_debug_interface{};

            const HRESULT query_result =
                debug_interface.As(
                    &advanced_debug_interface);

            if (FAILED(query_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        unsupported_operation,
                    failure_message(
                        "Querying ID3D12Debug1",
                        query_result));
            }

            advanced_debug_interface->
                SetEnableGPUBasedValidation(TRUE);

            return {};
        }

        [[nodiscard]] foundation::Status
        create_factory()
        {
            UINT factory_flags = 0U;

            if (config.enable_debug_layer)
            {
                factory_flags |=
                    DXGI_CREATE_FACTORY_DEBUG;
            }

            const HRESULT result =
                CreateDXGIFactory2(
                    factory_flags,
                    IID_PPV_ARGS(
                        factory.GetAddressOf()));

            if (FAILED(result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "CreateDXGIFactory2",
                        result));
            }

            return {};
        }

        void detect_tearing_support() noexcept
        {
            renderer_info.tearing_supported = false;

            ComPtr<IDXGIFactory5>
                factory_version_five{};

            if (FAILED(
                    factory.As(
                        &factory_version_five)))
            {
                return;
            }

            BOOL allow_tearing = FALSE;

            const HRESULT result =
                factory_version_five->
                    CheckFeatureSupport(
                        DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                        &allow_tearing,
                        sizeof(allow_tearing));

            renderer_info.tearing_supported =
                SUCCEEDED(result) &&
                allow_tearing == TRUE;
        }

        [[nodiscard]] bool
        adapter_supports_d3d12(
            IDXGIAdapter1* candidate) const
        {
            ComPtr<ID3D12Device> probe_device{};

            const HRESULT result =
                D3D12CreateDevice(
                    candidate,
                    D3D_FEATURE_LEVEL_11_0,
                    IID_PPV_ARGS(
                        probe_device.GetAddressOf()));

            return SUCCEEDED(result);
        }

        [[nodiscard]] foundation::Status
        select_adapter()
        {
            const DXGI_GPU_PREFERENCE preference =
                config.prefer_high_performance_adapter
                    ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE
                    : DXGI_GPU_PREFERENCE_UNSPECIFIED;

            for (UINT adapter_index = 0U;
                 ;
                 ++adapter_index)
            {
                ComPtr<IDXGIAdapter1> candidate{};

                const HRESULT enumeration_result =
                    factory->
                        EnumAdapterByGpuPreference(
                            adapter_index,
                            preference,
                            IID_PPV_ARGS(
                                candidate.GetAddressOf()));

                if (enumeration_result ==
                    DXGI_ERROR_NOT_FOUND)
                {
                    break;
                }

                if (FAILED(enumeration_result))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        failure_message(
                            "EnumAdapterByGpuPreference",
                            enumeration_result));
                }

                DXGI_ADAPTER_DESC1 description{};

                const HRESULT description_result =
                    candidate->GetDesc1(
                        &description);

                if (FAILED(description_result))
                {
                    continue;
                }

                if ((description.Flags &
                     DXGI_ADAPTER_FLAG_SOFTWARE) != 0U)
                {
                    continue;
                }

                if (!adapter_supports_d3d12(
                        candidate.Get()))
                {
                    continue;
                }

                adapter = std::move(candidate);
                break;
            }

            if (adapter == nullptr)
            {
                const HRESULT warp_result =
                    factory->EnumWarpAdapter(
                        IID_PPV_ARGS(
                            adapter.GetAddressOf()));

                if (FAILED(warp_result))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            unsupported_operation,
                        "No Direct3D 12-compatible "
                        "hardware or WARP adapter was "
                        "available. " +
                            failure_message(
                                "EnumWarpAdapter",
                                warp_result));
                }
            }

            DXGI_ADAPTER_DESC1 description{};

            const HRESULT description_result =
                adapter->GetDesc1(
                    &description);

            if (FAILED(description_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "IDXGIAdapter1::GetDesc1",
                        description_result));
            }

            renderer_info.adapter_name =
                wide_to_utf8(
                    description.Description);

            renderer_info.
                dedicated_video_memory_bytes =
                    static_cast<std::uint64_t>(
                        description.
                            DedicatedVideoMemory);

            renderer_info.
                shared_system_memory_bytes =
                    static_cast<std::uint64_t>(
                        description.
                            SharedSystemMemory);

            return {};
        }

        [[nodiscard]] foundation::Status
        create_device()
        {
            const HRESULT result =
                D3D12CreateDevice(
                    adapter.Get(),
                    D3D_FEATURE_LEVEL_11_0,
                    IID_PPV_ARGS(
                        device.GetAddressOf()));

            if (FAILED(result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        unsupported_operation,
                    failure_message(
                        "D3D12CreateDevice",
                        result));
            }

            return {};
        }

        [[nodiscard]] foundation::Status
        create_command_queue()
        {
            D3D12_COMMAND_QUEUE_DESC description{};
            description.Type =
                D3D12_COMMAND_LIST_TYPE_DIRECT;
            description.Priority =
                D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            description.Flags =
                D3D12_COMMAND_QUEUE_FLAG_NONE;
            description.NodeMask = 0U;

            const HRESULT result =
                device->CreateCommandQueue(
                    &description,
                    IID_PPV_ARGS(
                        command_queue.GetAddressOf()));

            if (FAILED(result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "ID3D12Device::"
                        "CreateCommandQueue",
                        result));
            }

            return {};
        }

        [[nodiscard]] foundation::Status
        create_swap_chain()
        {
            DXGI_SWAP_CHAIN_DESC1 description{};

            description.Width = width;
            description.Height = height;
            description.Format =
                DXGI_FORMAT_R8G8B8A8_UNORM;
            description.Stereo = FALSE;
            description.SampleDesc.Count = 1U;
            description.SampleDesc.Quality = 0U;
            description.BufferUsage =
                DXGI_USAGE_RENDER_TARGET_OUTPUT;
            description.BufferCount =
                config.frame_buffer_count;
            description.Scaling =
                DXGI_SCALING_STRETCH;
            description.SwapEffect =
                DXGI_SWAP_EFFECT_FLIP_DISCARD;
            description.AlphaMode =
                DXGI_ALPHA_MODE_IGNORE;
            description.Flags =
                swap_chain_flags(
                    config,
                    renderer_info.
                        tearing_supported);

            ComPtr<IDXGISwapChain1>
                initial_swap_chain{};

            const HRESULT creation_result =
                factory->CreateSwapChainForHwnd(
                    command_queue.Get(),
                    window_handle,
                    &description,
                    nullptr,
                    nullptr,
                    initial_swap_chain.
                        GetAddressOf());

            if (FAILED(creation_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "CreateSwapChainForHwnd",
                        creation_result));
            }

            const HRESULT association_result =
                factory->MakeWindowAssociation(
                    window_handle,
                    DXGI_MWA_NO_ALT_ENTER);

            if (FAILED(association_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "MakeWindowAssociation",
                        association_result));
            }

            const HRESULT query_result =
                initial_swap_chain.As(
                    &swap_chain);

            if (FAILED(query_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "Querying IDXGISwapChain4",
                        query_result));
            }

            frame_index =
                swap_chain->
                    GetCurrentBackBufferIndex();

            return {};
        }

        [[nodiscard]] foundation::Status
        create_render_target_view_heap()
        {
            D3D12_DESCRIPTOR_HEAP_DESC description{};

            description.Type =
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

            description.NumDescriptors =
                config.frame_buffer_count;

            description.Flags =
                D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            description.NodeMask = 0U;

            const HRESULT result =
                device->CreateDescriptorHeap(
                    &description,
                    IID_PPV_ARGS(
                        render_target_view_heap.
                            GetAddressOf()));

            if (FAILED(result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "CreateDescriptorHeap",
                        result));
            }

            render_target_view_descriptor_size =
                device->
                    GetDescriptorHandleIncrementSize(
                        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            return {};
        }

        [[nodiscard]] foundation::Status
        create_command_allocators()
        {
            command_allocators.clear();
            command_allocators.resize(
                config.frame_buffer_count);

            for (std::uint32_t index = 0U;
                 index < config.frame_buffer_count;
                 ++index)
            {
                const HRESULT result =
                    device->CreateCommandAllocator(
                        D3D12_COMMAND_LIST_TYPE_DIRECT,
                        IID_PPV_ARGS(
                            command_allocators[index].
                                GetAddressOf()));

                if (FAILED(result))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        failure_message(
                            "CreateCommandAllocator",
                            result));
                }
            }

            return {};
        }

        [[nodiscard]] foundation::Status
        create_command_list()
        {
            const HRESULT creation_result =
                device->CreateCommandList(
                    0U,
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    command_allocators[0U].Get(),
                    nullptr,
                    IID_PPV_ARGS(
                        command_list.GetAddressOf()));

            if (FAILED(creation_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "CreateCommandList",
                        creation_result));
            }

            const HRESULT close_result =
                command_list->Close();

            if (FAILED(close_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "Closing the initial "
                        "command list",
                        close_result));
            }

            return {};
        }

        [[nodiscard]] foundation::Status
        create_render_targets()
        {
            render_targets.clear();
            render_targets.resize(
                config.frame_buffer_count);

            D3D12_CPU_DESCRIPTOR_HANDLE
                descriptor_handle =
                    render_target_view_heap->
                        GetCPUDescriptorHandleForHeapStart();

            for (std::uint32_t index = 0U;
                 index < config.frame_buffer_count;
                 ++index)
            {
                const HRESULT result =
                    swap_chain->GetBuffer(
                        index,
                        IID_PPV_ARGS(
                            render_targets[index].
                                GetAddressOf()));

                if (FAILED(result))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        failure_message(
                            "IDXGISwapChain::GetBuffer",
                            result));
                }

                device->CreateRenderTargetView(
                    render_targets[index].Get(),
                    nullptr,
                    descriptor_handle);

                descriptor_handle.ptr +=
                    static_cast<SIZE_T>(
                        render_target_view_descriptor_size);
            }

            return {};
        }

        [[nodiscard]] foundation::Status
        create_synchronization_objects()
        {
            const HRESULT fence_result =
                device->CreateFence(
                    0U,
                    D3D12_FENCE_FLAG_NONE,
                    IID_PPV_ARGS(
                        fence.GetAddressOf()));

            if (FAILED(fence_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "CreateFence",
                        fence_result));
            }

            fence_event =
                CreateEventW(
                    nullptr,
                    FALSE,
                    FALSE,
                    nullptr);

            if (fence_event == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "CreateEventW failed while "
                    "creating the GPU fence event.");
            }

            frame_fence_values.assign(
                config.frame_buffer_count,
                0U);

            next_fence_value = 1U;

            return {};
        }

        [[nodiscard]] foundation::Status
        wait_for_fence_value(
            const std::uint64_t value)
        {
            if (value == 0U ||
                fence->GetCompletedValue() >= value)
            {
                return {};
            }

            const HRESULT event_result =
                fence->SetEventOnCompletion(
                    value,
                    fence_event);

            if (FAILED(event_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "SetEventOnCompletion",
                        event_result));
            }

            const DWORD wait_result =
                WaitForSingleObject(
                    fence_event,
                    INFINITE);

            if (wait_result != WAIT_OBJECT_0)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "WaitForSingleObject failed while "
                    "waiting for the GPU fence.");
            }

            return {};
        }

        [[nodiscard]] foundation::Status
        signal_and_advance_frame()
        {
            const std::uint64_t signal_value =
                next_fence_value;

            ++next_fence_value;

            const HRESULT signal_result =
                command_queue->Signal(
                    fence.Get(),
                    signal_value);

            if (FAILED(signal_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "ID3D12CommandQueue::Signal",
                        signal_result));
            }

            frame_fence_values[frame_index] =
                signal_value;

            frame_index =
                swap_chain->
                    GetCurrentBackBufferIndex();

            return wait_for_fence_value(
                frame_fence_values[frame_index]);
        }

        [[nodiscard]] foundation::Status
        draw(const ClearColor& clear_color)
        {
            if (!ready)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Renderer is not ready.");
            }

            const HRESULT allocator_result =
                command_allocators[frame_index]->
                    Reset();

            if (FAILED(allocator_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "ID3D12CommandAllocator::Reset",
                        allocator_result));
            }

            const HRESULT list_reset_result =
                command_list->Reset(
                    command_allocators[
                        frame_index].Get(),
                    nullptr);

            if (FAILED(list_reset_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "ID3D12GraphicsCommandList::"
                        "Reset",
                        list_reset_result));
            }

            D3D12_RESOURCE_BARRIER
                to_render_target{};

            to_render_target.Type =
                D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

            to_render_target.Flags =
                D3D12_RESOURCE_BARRIER_FLAG_NONE;

            to_render_target.Transition.pResource =
                render_targets[frame_index].Get();

            to_render_target.Transition.Subresource =
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            to_render_target.
                Transition.StateBefore =
                    D3D12_RESOURCE_STATE_PRESENT;

            to_render_target.
                Transition.StateAfter =
                    D3D12_RESOURCE_STATE_RENDER_TARGET;

            command_list->ResourceBarrier(
                1U,
                &to_render_target);

            D3D12_CPU_DESCRIPTOR_HANDLE
                render_target_handle =
                    render_target_view_heap->
                        GetCPUDescriptorHandleForHeapStart();

            render_target_handle.ptr +=
                static_cast<SIZE_T>(
                    frame_index) *
                static_cast<SIZE_T>(
                    render_target_view_descriptor_size);

            command_list->OMSetRenderTargets(
                1U,
                &render_target_handle,
                FALSE,
                nullptr);

            const float color[4]{
                clear_color.red,
                clear_color.green,
                clear_color.blue,
                clear_color.alpha
            };

            command_list->ClearRenderTargetView(
                render_target_handle,
                color,
                0U,
                nullptr);

            D3D12_RESOURCE_BARRIER
                to_present{};

            to_present.Type =
                D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

            to_present.Flags =
                D3D12_RESOURCE_BARRIER_FLAG_NONE;

            to_present.Transition.pResource =
                render_targets[frame_index].Get();

            to_present.Transition.Subresource =
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            to_present.Transition.StateBefore =
                D3D12_RESOURCE_STATE_RENDER_TARGET;

            to_present.Transition.StateAfter =
                D3D12_RESOURCE_STATE_PRESENT;

            command_list->ResourceBarrier(
                1U,
                &to_present);

            const HRESULT close_result =
                command_list->Close();

            if (FAILED(close_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "ID3D12GraphicsCommandList::"
                        "Close",
                        close_result));
            }

            ID3D12CommandList* command_lists[]{
                command_list.Get()
            };

            command_queue->ExecuteCommandLists(
                1U,
                command_lists);

            const UINT synchronization_interval =
                config.vertical_synchronization
                    ? 1U
                    : 0U;

            const HRESULT present_result =
                swap_chain->Present(
                    synchronization_interval,
                    present_flags(
                        config,
                        renderer_info.
                            tearing_supported));

            if (FAILED(present_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "IDXGISwapChain::Present",
                        present_result));
            }

            return signal_and_advance_frame();
        }

        [[nodiscard]] foundation::Status
        resize_buffers(
            const std::uint32_t new_width,
            const std::uint32_t new_height)
        {
            if (!ready)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Renderer is not ready.");
            }

            if (new_width == 0U ||
                new_height == 0U)
            {
                return {};
            }

            if (new_width == width &&
                new_height == height)
            {
                return {};
            }

            foundation::Status idle_status =
                wait_until_idle();

            if (!idle_status.has_value())
            {
                return idle_status;
            }

            for (ComPtr<ID3D12Resource>& target :
                 render_targets)
            {
                target.Reset();
            }

            const HRESULT resize_result =
                swap_chain->ResizeBuffers(
                    config.frame_buffer_count,
                    new_width,
                    new_height,
                    DXGI_FORMAT_R8G8B8A8_UNORM,
                    swap_chain_flags(
                        config,
                        renderer_info.
                            tearing_supported));

            if (FAILED(resize_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "IDXGISwapChain::ResizeBuffers",
                        resize_result));
            }

            width = new_width;
            height = new_height;

            frame_index =
                swap_chain->
                    GetCurrentBackBufferIndex();

            frame_fence_values.assign(
                config.frame_buffer_count,
                0U);

            return create_render_targets();
        }

        [[nodiscard]] foundation::Status
        wait_until_idle()
        {
            if (!ready)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Renderer is not ready.");
            }

            const std::uint64_t signal_value =
                next_fence_value;

            ++next_fence_value;

            const HRESULT signal_result =
                command_queue->Signal(
                    fence.Get(),
                    signal_value);

            if (FAILED(signal_result))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    failure_message(
                        "ID3D12CommandQueue::Signal",
                        signal_result));
            }

            return wait_for_fence_value(
                signal_value);
        }

        void shutdown() noexcept
        {
            if (command_queue != nullptr &&
                fence != nullptr &&
                fence_event != nullptr)
            {
                const std::uint64_t signal_value =
                    next_fence_value;

                ++next_fence_value;

                if (SUCCEEDED(
                        command_queue->Signal(
                            fence.Get(),
                            signal_value)))
                {
                    if (fence->
                            GetCompletedValue() <
                        signal_value)
                    {
                        if (SUCCEEDED(
                                fence->
                                    SetEventOnCompletion(
                                        signal_value,
                                        fence_event)))
                        {
                            static_cast<void>(
                                WaitForSingleObject(
                                    fence_event,
                                    INFINITE));
                        }
                    }
                }
            }

            ready = false;

            command_list.Reset();
            command_allocators.clear();
            render_targets.clear();

            render_target_view_heap.Reset();
            swap_chain.Reset();
            command_queue.Reset();
            fence.Reset();
            device.Reset();
            adapter.Reset();
            factory.Reset();

            if (fence_event != nullptr)
            {
                CloseHandle(fence_event);
                fence_event = nullptr;
            }
        }
    };

    Renderer::Renderer(
        std::unique_ptr<Implementation>
            implementation) noexcept
        : implementation_{
            std::move(implementation)}
    {
    }

    Renderer::~Renderer() = default;

    Renderer::Renderer(
        Renderer&& other) noexcept = default;

    Renderer& Renderer::operator=(
        Renderer&& other) noexcept = default;

    foundation::Result<Renderer>
    Renderer::create(
        void* const native_window_handle,
        RendererConfig config)
    {
        auto implementation =
            std::make_unique<Implementation>();

        implementation->window_handle =
            static_cast<HWND>(
                native_window_handle);

        implementation->config =
            std::move(config);

        foundation::Status status =
            implementation->initialize();

        if (!status.has_value())
        {
            return foundation::fail(
                status.error().code,
                status.error().message);
        }

        return Renderer{
            std::move(implementation)};
    }

    foundation::Status
    Renderer::draw_frame(
        const ClearColor& clear_color)
    {
        if (implementation_ == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Renderer has no implementation.");
        }

        return implementation_->draw(
            clear_color);
    }

    foundation::Status
    Renderer::resize(
        const std::uint32_t width,
        const std::uint32_t height)
    {
        if (implementation_ == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Renderer has no implementation.");
        }

        return implementation_->resize_buffers(
            width,
            height);
    }

    foundation::Status
    Renderer::wait_idle()
    {
        if (implementation_ == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Renderer has no implementation.");
        }

        return implementation_->wait_until_idle();
    }

    bool Renderer::is_ready() const noexcept
    {
        return implementation_ != nullptr &&
            implementation_->ready;
    }

    const RendererInfo&
    Renderer::info() const noexcept
    {
        static const RendererInfo empty_info{};

        if (implementation_ == nullptr)
        {
            return empty_info;
        }

        return implementation_->renderer_info;
    }
}