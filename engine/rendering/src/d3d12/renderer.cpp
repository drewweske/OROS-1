#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "oros/rendering/renderer.hpp"

#include <Windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
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

        // Debug-world PSO and upload buffers live with this renderer.
        ComPtr<ID3D12RootSignature> debug_root_signature{};
        ComPtr<ID3D12PipelineState> debug_pipeline{};
        ComPtr<ID3D12PipelineState> solid_pipeline{};
        std::vector<ComPtr<ID3D12Resource>>
            debug_vertex_buffers{};
        std::vector<ComPtr<ID3D12Resource>>
            solid_vertex_buffers{};

        ComPtr<ID3D12DescriptorHeap> depth_view_heap{};
        ComPtr<ID3D12Resource> depth_buffer{};

        static constexpr std::size_t max_debug_vertices = 1024U;
        static constexpr std::size_t max_solid_vertices = 2048U;

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

            status = create_debug_world_pipeline();

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

        // Minimal programmable line pipeline used only for the
        // first graybox world. No gameplay state enters D3D12.
        [[nodiscard]] foundation::Status
        create_debug_world_pipeline()
        {
            static constexpr char shader_source[] = R"(
struct Input
{
    float2 position : POSITION;
    float4 color : COLOR;
    float depth : TEXCOORD;
};
struct Output
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};
Output VSMain(Input input)
{
    Output output;
    output.position = float4(input.position, input.depth, 1.0);
    output.color = input.color;
    return output;
}
float4 PSMain(Output input) : SV_TARGET
{
    return input.color;
}
)";

            ComPtr<ID3DBlob> vertex_shader{};
            ComPtr<ID3DBlob> pixel_shader{};
            ComPtr<ID3DBlob> errors{};

            HRESULT hr = D3DCompile(
                shader_source,
                sizeof(shader_source) - 1U,
                nullptr, nullptr, nullptr,
                "VSMain", "vs_5_0", 0U, 0U,
                vertex_shader.GetAddressOf(),
                errors.GetAddressOf());

            if (FAILED(hr))
            {
                return foundation::fail(
                    foundation::ErrorCode::internal_failure,
                    failure_message("Compile debug-world VS", hr));
            }

            errors.Reset();
            hr = D3DCompile(
                shader_source,
                sizeof(shader_source) - 1U,
                nullptr, nullptr, nullptr,
                "PSMain", "ps_5_0", 0U, 0U,
                pixel_shader.GetAddressOf(),
                errors.GetAddressOf());

            if (FAILED(hr))
            {
                return foundation::fail(
                    foundation::ErrorCode::internal_failure,
                    failure_message("Compile debug-world PS", hr));
            }

            D3D12_ROOT_SIGNATURE_DESC root_description{};
            root_description.Flags =
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            ComPtr<ID3DBlob> serialized_root{};
            errors.Reset();
            hr = D3D12SerializeRootSignature(
                &root_description,
                D3D_ROOT_SIGNATURE_VERSION_1,
                serialized_root.GetAddressOf(),
                errors.GetAddressOf());

            if (FAILED(hr))
            {
                return foundation::fail(
                    foundation::ErrorCode::internal_failure,
                    failure_message("Serialize debug-world root signature", hr));
            }

            hr = device->CreateRootSignature(
                0U,
                serialized_root->GetBufferPointer(),
                serialized_root->GetBufferSize(),
                IID_PPV_ARGS(debug_root_signature.GetAddressOf()));

            if (FAILED(hr))
            {
                return foundation::fail(
                    foundation::ErrorCode::internal_failure,
                    failure_message("Create debug-world root signature", hr));
            }

            D3D12_INPUT_ELEMENT_DESC inputs[3]{};
            inputs[0].SemanticName = "POSITION";
            inputs[0].Format = DXGI_FORMAT_R32G32_FLOAT;
            inputs[0].InputSlotClass =
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            inputs[1].SemanticName = "COLOR";
            inputs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            inputs[1].AlignedByteOffset =
                static_cast<UINT>(offsetof(DebugWorldVertex, red));
            inputs[1].InputSlotClass =
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            inputs[2].SemanticName = "TEXCOORD";
            inputs[2].Format = DXGI_FORMAT_R32_FLOAT;
            inputs[2].AlignedByteOffset =
                static_cast<UINT>(offsetof(DebugWorldVertex, depth));
            inputs[2].InputSlotClass =
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

            D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};
            pipeline.pRootSignature = debug_root_signature.Get();
            pipeline.VS = {
                vertex_shader->GetBufferPointer(),
                vertex_shader->GetBufferSize()
            };
            pipeline.PS = {
                pixel_shader->GetBufferPointer(),
                pixel_shader->GetBufferSize()
            };
            pipeline.InputLayout = {inputs, 3U};
            pipeline.BlendState.AlphaToCoverageEnable = FALSE;
            pipeline.BlendState.IndependentBlendEnable = FALSE;

            D3D12_RENDER_TARGET_BLEND_DESC blend{};
            blend.BlendEnable = FALSE;
            blend.LogicOpEnable = FALSE;
            blend.SrcBlend = D3D12_BLEND_ONE;
            blend.DestBlend = D3D12_BLEND_ZERO;
            blend.BlendOp = D3D12_BLEND_OP_ADD;
            blend.SrcBlendAlpha = D3D12_BLEND_ONE;
            blend.DestBlendAlpha = D3D12_BLEND_ZERO;
            blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
            blend.LogicOp = D3D12_LOGIC_OP_NOOP;
            blend.RenderTargetWriteMask =
                D3D12_COLOR_WRITE_ENABLE_ALL;
            pipeline.BlendState.RenderTarget[0] = blend;

            pipeline.SampleMask = (std::numeric_limits<UINT>::max)();
            pipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            pipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
            pipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
            pipeline.RasterizerState.DepthBiasClamp =
                D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
            pipeline.RasterizerState.SlopeScaledDepthBias =
                D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
            pipeline.RasterizerState.DepthClipEnable = TRUE;
            pipeline.DepthStencilState.DepthEnable = TRUE;
            pipeline.DepthStencilState.DepthWriteMask =
                D3D12_DEPTH_WRITE_MASK_ZERO;
            pipeline.DepthStencilState.DepthFunc =
                D3D12_COMPARISON_FUNC_LESS_EQUAL;
            pipeline.DepthStencilState.StencilEnable = FALSE;
            pipeline.PrimitiveTopologyType =
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            pipeline.NumRenderTargets = 1U;
            pipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            pipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
            pipeline.SampleDesc.Count = 1U;

            hr = device->CreateGraphicsPipelineState(
                &pipeline,
                IID_PPV_ARGS(debug_pipeline.GetAddressOf()));
            if (FAILED(hr))
            {
                return foundation::fail(
                    foundation::ErrorCode::internal_failure,
                    failure_message("Create debug-world pipeline", hr));
            }

            // Opaque faces write depth; wireframe overlays only test.
            D3D12_INPUT_ELEMENT_DESC solid_inputs[3]{};
            solid_inputs[0] = inputs[0];
            solid_inputs[1] = inputs[1];
            solid_inputs[1].AlignedByteOffset =
                static_cast<UINT>(
                    offsetof(DebugWorldSolidVertex, red));
            solid_inputs[2] = inputs[2];
            solid_inputs[2].AlignedByteOffset =
                static_cast<UINT>(
                    offsetof(DebugWorldSolidVertex, depth));

            pipeline.InputLayout = {solid_inputs, 3U};
            pipeline.PrimitiveTopologyType =
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            pipeline.DepthStencilState.DepthWriteMask =
                D3D12_DEPTH_WRITE_MASK_ALL;
            hr = device->CreateGraphicsPipelineState(
                &pipeline,
                IID_PPV_ARGS(solid_pipeline.GetAddressOf()));
            if (FAILED(hr))
            {
                return foundation::fail(
                    foundation::ErrorCode::internal_failure,
                    failure_message("Create solid-world pipeline", hr));
            }

            D3D12_DESCRIPTOR_HEAP_DESC dsv_heap{};
            dsv_heap.NumDescriptors = 1U;
            dsv_heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            hr = device->CreateDescriptorHeap(
                &dsv_heap,
                IID_PPV_ARGS(depth_view_heap.GetAddressOf()));
            if (FAILED(hr))
            {
                return foundation::fail(
                    foundation::ErrorCode::internal_failure,
                    failure_message("Create world depth descriptor heap", hr));
            }

            D3D12_HEAP_PROPERTIES heap{};
            heap.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC buffer{};
            buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            buffer.Width =
                sizeof(DebugWorldVertex) * max_debug_vertices;
            buffer.Height = 1U;
            buffer.DepthOrArraySize = 1U;
            buffer.MipLevels = 1U;
            buffer.SampleDesc.Count = 1U;
            buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            debug_vertex_buffers.resize(config.frame_buffer_count);
            for (auto& resource : debug_vertex_buffers)
            {
                hr = device->CreateCommittedResource(
                    &heap,
                    D3D12_HEAP_FLAG_NONE,
                    &buffer,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(resource.GetAddressOf()));
                if (FAILED(hr))
                {
                    return foundation::fail(
                        foundation::ErrorCode::out_of_memory,
                        failure_message("Create debug-world vertex buffer", hr));
                }
            }

            buffer.Width =
                sizeof(DebugWorldSolidVertex) * max_solid_vertices;
            solid_vertex_buffers.resize(config.frame_buffer_count);
            for (auto& resource : solid_vertex_buffers)
            {
                hr = device->CreateCommittedResource(
                    &heap,
                    D3D12_HEAP_FLAG_NONE,
                    &buffer,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(resource.GetAddressOf()));
                if (FAILED(hr))
                {
                    return foundation::fail(
                        foundation::ErrorCode::out_of_memory,
                        failure_message("Create solid-world vertex buffer", hr));
                }
            }

            return {};
        }

        [[nodiscard]] foundation::Status
        draw_debug_world(
            const DebugWorldScene& scene)
        {
            const float aspect =
                static_cast<float>(width) /
                static_cast<float>(height);
            auto geometry = build_debug_world_lines(scene, aspect);
            if (!geometry.has_value())
            {
                return foundation::fail(
                    geometry.error().code,
                    geometry.error().message);
            }

            if (geometry.value().size() > max_debug_vertices)
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Debug-world vertex count exceeds GPU buffer.");
            }

            void* mapped{};
            const D3D12_RANGE read_range{0U, 0U};
            const HRESULT map_result =
                debug_vertex_buffers[frame_index]->Map(
                    0U, &read_range, &mapped);
            if (FAILED(map_result) || mapped == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::internal_failure,
                    failure_message("Map debug-world vertex buffer", map_result));
            }

            const std::size_t bytes =
                geometry.value().size() * sizeof(DebugWorldVertex);
            std::memcpy(mapped, geometry.value().data(), bytes);
            const D3D12_RANGE written{0U, bytes};
            debug_vertex_buffers[frame_index]->Unmap(0U, &written);

            const D3D12_VIEWPORT viewport{
                0.0F, 0.0F,
                static_cast<float>(width),
                static_cast<float>(height),
                0.0F, 1.0F
            };
            const D3D12_RECT scissor{
                0L, 0L,
                static_cast<LONG>(width),
                static_cast<LONG>(height)
            };
            command_list->RSSetViewports(1U, &viewport);
            command_list->RSSetScissorRects(1U, &scissor);
            command_list->SetGraphicsRootSignature(
                debug_root_signature.Get());
            command_list->SetPipelineState(
                debug_pipeline.Get());
            command_list->IASetPrimitiveTopology(
                D3D_PRIMITIVE_TOPOLOGY_LINELIST);

            const D3D12_VERTEX_BUFFER_VIEW view{
                debug_vertex_buffers[frame_index]->GetGPUVirtualAddress(),
                static_cast<UINT>(bytes),
                static_cast<UINT>(sizeof(DebugWorldVertex))
            };
            command_list->IASetVertexBuffers(0U, 1U, &view);
            command_list->DrawInstanced(
                static_cast<UINT>(geometry.value().size()),
                1U, 0U, 0U);

            return {};
        }

        [[nodiscard]] foundation::Status
        draw_solid_world(const DebugWorldScene& scene)
        {
            const float aspect =
                static_cast<float>(width) /
                static_cast<float>(height);
            const auto geometry =
                build_debug_world_solids(scene, aspect);
            if (!geometry.has_value())
            {
                return foundation::fail(
                    geometry.error().code,
                    geometry.error().message);
            }
            if (geometry.value().size() > max_solid_vertices)
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Solid-world vertex count exceeds GPU buffer.");
            }

            void* mapped{};
            const D3D12_RANGE read_range{0U, 0U};
            const HRESULT hr =
                solid_vertex_buffers[frame_index]->Map(
                    0U, &read_range, &mapped);
            if (FAILED(hr) || mapped == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::internal_failure,
                    failure_message("Map solid-world vertex buffer", hr));
            }
            const std::size_t bytes =
                geometry.value().size() * sizeof(DebugWorldSolidVertex);
            std::memcpy(mapped, geometry.value().data(), bytes);
            const D3D12_RANGE written{0U, bytes};
            solid_vertex_buffers[frame_index]->Unmap(0U, &written);

            const D3D12_VIEWPORT viewport{
                0.0F, 0.0F,
                static_cast<float>(width),
                static_cast<float>(height), 0.0F, 1.0F
            };
            const D3D12_RECT scissor{
                0L, 0L, static_cast<LONG>(width),
                static_cast<LONG>(height)
            };
            command_list->RSSetViewports(1U, &viewport);
            command_list->RSSetScissorRects(1U, &scissor);
            command_list->SetGraphicsRootSignature(
                debug_root_signature.Get());
            command_list->SetPipelineState(solid_pipeline.Get());
            command_list->IASetPrimitiveTopology(
                D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            const D3D12_VERTEX_BUFFER_VIEW view{
                solid_vertex_buffers[frame_index]->GetGPUVirtualAddress(),
                static_cast<UINT>(bytes),
                static_cast<UINT>(sizeof(DebugWorldSolidVertex))
            };
            command_list->IASetVertexBuffers(0U, 1U, &view);
            command_list->DrawInstanced(
                static_cast<UINT>(geometry.value().size()),
                1U, 0U, 0U);
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

            // Depth target shares the swapchain dimensions and is
            // recreated after a GPU-idle resize.
            depth_buffer.Reset();
            D3D12_HEAP_PROPERTIES heap{};
            heap.Type = D3D12_HEAP_TYPE_DEFAULT;
            D3D12_RESOURCE_DESC desc{};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Width = width;
            desc.Height = height;
            desc.DepthOrArraySize = 1U;
            desc.MipLevels = 1U;
            desc.Format = DXGI_FORMAT_D32_FLOAT;
            desc.SampleDesc.Count = 1U;
            desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            D3D12_CLEAR_VALUE clear{};
            clear.Format = DXGI_FORMAT_D32_FLOAT;
            clear.DepthStencil.Depth = 1.0F;
            const HRESULT depth_hr = device->CreateCommittedResource(
                &heap, D3D12_HEAP_FLAG_NONE,
                &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clear, IID_PPV_ARGS(depth_buffer.GetAddressOf()));
            if (FAILED(depth_hr))
            {
                return foundation::fail(
                    foundation::ErrorCode::internal_failure,
                    failure_message("Create world depth texture", depth_hr));
            }
            D3D12_DEPTH_STENCIL_VIEW_DESC view{};
            view.Format = DXGI_FORMAT_D32_FLOAT;
            view.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            device->CreateDepthStencilView(
                depth_buffer.Get(), &view,
                depth_view_heap->GetCPUDescriptorHandleForHeapStart());
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
        draw(
            const ClearColor& clear_color,
            const DebugWorldScene* scene)
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

            D3D12_CPU_DESCRIPTOR_HANDLE depth_handle =
                depth_view_heap->GetCPUDescriptorHandleForHeapStart();
            command_list->OMSetRenderTargets(
                1U,
                &render_target_handle,
                FALSE,
                scene == nullptr ? nullptr : &depth_handle);

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

            if (scene != nullptr)
            {
                command_list->ClearDepthStencilView(
                    depth_handle, D3D12_CLEAR_FLAG_DEPTH,
                    1.0F, 0U, 0U, nullptr);
                const foundation::Status solid_status =
                    draw_solid_world(*scene);
                if (!solid_status.has_value())
                {
                    return solid_status;
                }

                // Depth-tested colored silhouettes first; overlay
                // the old wireframe, grid and NPC posts afterward.
                const foundation::Status outline_status =
                    draw_debug_world(*scene);
                if (!outline_status.has_value())
                {
                    return outline_status;
                }
            }

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
            depth_buffer.Reset();

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
            debug_vertex_buffers.clear();
            solid_vertex_buffers.clear();
            debug_pipeline.Reset();
            solid_pipeline.Reset();
            debug_root_signature.Reset();
            depth_buffer.Reset();
            depth_view_heap.Reset();
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
            clear_color, nullptr);
    }

    foundation::Status
    Renderer::draw_world_frame(
        const ClearColor& clear_color,
        const DebugWorldScene& scene)
    {
        if (implementation_ == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "Renderer has no implementation.");
        }

        return implementation_->draw(clear_color, &scene);
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