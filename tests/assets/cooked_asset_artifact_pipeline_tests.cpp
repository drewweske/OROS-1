#include "oros/assets/cooked_asset_artifact_pipeline.hpp"

#include "oros/assets/content_hash.hpp"
#include "oros/assets/run_length_compression_codec.hpp"
#include "oros/foundation/error.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    struct TestState final
    {
        int checks{};
        int failures{};
    };

    void check(
        TestState& state,
        const bool condition,
        const std::string_view name)
    {
        ++state.checks;

        if (condition)
        {
            std::cout
                << "[pass] "
                << name
                << '\n';

            return;
        }

        ++state.failures;

        std::cerr
            << "[fail] "
            << name
            << '\n';
    }

    template <typename T>
    void check_failure(
        TestState& state,
        const oros::foundation::Result<T>& result,
        const oros::foundation::ErrorCode
            expected_code,
        const std::string_view name)
    {
        const bool passed =
            !result.has_value() &&
            result.error().code ==
                expected_code;

        check(
            state,
            passed,
            name);
    }

    enum class TestCodecBehavior
        : std::uint8_t
    {
        identity = 0U,
        fail_compression,
        fail_decompression,
        invalid_compression_result,
        wrong_original_byte_count,
        wrong_decoded_byte_count,
        mutate_during_compression,
        mutate_during_decompression,
        throw_during_compression,
        throw_during_decompression
    };

    class TestCompressionCodec final
        : public oros::assets::
            AssetCompressionCodec
    {
    public:
        explicit TestCompressionCodec(
            const TestCodecBehavior behavior =
                TestCodecBehavior::identity,
            const std::string_view stable_name =
                "test.identity",
            const std::uint32_t stable_version =
                7U) noexcept
            : behavior_{behavior},
              stable_name_{stable_name},
              stable_version_{stable_version}
        {
        }

        [[nodiscard]]
        std::string_view
        name() const noexcept override
        {
            return
                metadata_changed_
                    ? std::string_view{
                          "test.mutated"
                      }
                    : stable_name_;
        }

        [[nodiscard]]
        std::uint32_t
        version() const noexcept override
        {
            return stable_version_;
        }

        [[nodiscard]]
        oros::foundation::Result<
            oros::assets::
                AssetCompressionResult>
        compress(
            const oros::assets::
                AssetCompressionRequest&
                    request) const override
        {
            using namespace oros;

            if (!request.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "The test compression request "
                    "is invalid.");
            }

            switch (behavior_)
            {
            case TestCodecBehavior::
                fail_compression:
                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "Forced compression failure.");

            case TestCodecBehavior::
                invalid_compression_result:
                return assets::
                    AssetCompressionResult{
                        {},
                        request.source_bytes.size()
                    };

            case TestCodecBehavior::
                wrong_original_byte_count:
                return assets::
                    AssetCompressionResult{
                        copy_bytes(
                            request.source_bytes),
                        request.source_bytes.size() +
                            1U
                    };

            case TestCodecBehavior::
                mutate_during_compression:
                metadata_changed_ = true;
                break;

            case TestCodecBehavior::
                throw_during_compression:
                throw std::runtime_error{
                    "Forced compression exception."
                };

            case TestCodecBehavior::identity:
            case TestCodecBehavior::
                fail_decompression:
            case TestCodecBehavior::
                wrong_decoded_byte_count:
            case TestCodecBehavior::
                mutate_during_decompression:
            case TestCodecBehavior::
                throw_during_decompression:
                break;
            }

            return assets::
                AssetCompressionResult{
                    copy_bytes(
                        request.source_bytes),
                    request.source_bytes.size()
                };
        }

        [[nodiscard]]
        oros::foundation::Result<
            std::vector<std::byte>>
        decompress(
            const oros::assets::
                AssetDecompressionRequest&
                    request) const override
        {
            using namespace oros;

            if (!request.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "The test decompression request "
                    "is invalid.");
            }

            switch (behavior_)
            {
            case TestCodecBehavior::
                fail_decompression:
                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "Forced decompression failure.");

            case TestCodecBehavior::
                wrong_decoded_byte_count:
            {
                std::vector<std::byte> bytes =
                    copy_bytes(
                        request.compressed_bytes);

                if (bytes.empty())
                {
                    bytes.push_back(
                        std::byte{0x00});
                }
                else
                {
                    bytes.pop_back();
                }

                return bytes;
            }

            case TestCodecBehavior::
                mutate_during_decompression:
                metadata_changed_ = true;
                break;

            case TestCodecBehavior::
                throw_during_decompression:
                throw std::runtime_error{
                    "Forced decompression exception."
                };

            case TestCodecBehavior::identity:
            case TestCodecBehavior::
                fail_compression:
            case TestCodecBehavior::
                invalid_compression_result:
            case TestCodecBehavior::
                wrong_original_byte_count:
            case TestCodecBehavior::
                mutate_during_compression:
            case TestCodecBehavior::
                throw_during_compression:
                break;
            }

            std::vector<std::byte> bytes =
                copy_bytes(
                    request.compressed_bytes);

            if (bytes.size() !=
                request.expected_byte_count)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "The test decompression byte "
                    "count does not match.");
            }

            return bytes;
        }

    private:
        [[nodiscard]]
        static std::vector<std::byte>
        copy_bytes(
            const std::span<const std::byte>
                bytes)
        {
            return std::vector<std::byte>{
                bytes.begin(),
                bytes.end()
            };
        }

        TestCodecBehavior behavior_{};
        std::string_view stable_name_{};
        std::uint32_t stable_version_{};
        mutable bool metadata_changed_{};
    };

    [[nodiscard]]
    oros::assets::CookedAsset
    make_cooked_asset(
        const std::span<const std::byte> bytes)
    {
        using namespace oros::assets;

        CookedAsset asset{};

        asset.cooked_bytes = {
            bytes.begin(),
            bytes.end()
        };

        asset.record.id =
            AssetId{
                0x4F524F53ULL,
                300ULL
            };

        asset.record.source_path =
            "textures/terrain/grass.png";

        asset.record.importer_name =
            "oros.texture";

        asset.record.importer_version = 2U;
        asset.record.schema_version = 5U;

        asset.record.source_hash =
            hash_text(
                "source grass texture");

        asset.record.cooked_hash =
            hash_bytes(
                std::span<const std::byte>{
                    asset.cooked_bytes
                });

        asset.record.dependencies = {
            AssetId{
                0x4F524F53ULL,
                301ULL
            }
        };

        asset.cooker_name =
            "oros.texture";

        asset.cooker_version = 4U;

        return asset;
    }

    [[nodiscard]]
    bool cooked_assets_equal(
        const oros::assets::CookedAsset& left,
        const oros::assets::CookedAsset& right)
    {
        return
            left.record == right.record &&
            left.cooker_name ==
                right.cooker_name &&
            left.cooker_version ==
                right.cooker_version &&
            left.cooked_bytes ==
                right.cooked_bytes;
    }
}

int main()
{
    using namespace oros::assets;
    using oros::foundation::ErrorCode;

    TestState state{};

    const std::vector<std::byte>
        compressible_bytes(
            64U,
            std::byte{0x5A});

    const std::vector<std::byte>
        incompressible_bytes{
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x50},
            std::byte{0x60},
            std::byte{0x70},
            std::byte{0x80}
        };

    const std::vector<std::byte>
        empty_bytes{};

    const CookedAsset compressible_asset =
        make_cooked_asset(
            std::span<const std::byte>{
                compressible_bytes
            });

    const CookedAsset incompressible_asset =
        make_cooked_asset(
            std::span<const std::byte>{
                incompressible_bytes
            });

    const CookedAsset empty_asset =
        make_cooked_asset(
            std::span<const std::byte>{
                empty_bytes
            });

    RunLengthCompressionCodec rle_codec{};
    TestCompressionCodec identity_codec{};

    check(
        state,
        !CookedAssetArtifactBuildRequest{}.
            is_valid(),
        "Default artifact build request is invalid");

    const CookedAssetArtifactBuildRequest
        disabled_request{
            &compressible_asset,
            nullptr,
            CookedAssetArtifactCompressionPolicy::
                disabled
        };

    check(
        state,
        disabled_request.is_valid(),
        "Disabled compression request accepts no codec");

    check(
        state,
        !CookedAssetArtifactBuildRequest{
            &compressible_asset,
            &rle_codec,
            CookedAssetArtifactCompressionPolicy::
                disabled
        }.is_valid(),
        "Disabled compression request rejects a codec");

    check(
        state,
        !CookedAssetArtifactBuildRequest{
            &compressible_asset,
            nullptr,
            CookedAssetArtifactCompressionPolicy::
                when_smaller
        }.is_valid(),
        "Conditional compression requires a codec");

    check(
        state,
        CookedAssetArtifactBuildRequest{
            &compressible_asset,
            &rle_codec,
            CookedAssetArtifactCompressionPolicy::
                required
        }.is_valid(),
        "Required compression accepts a valid codec");

    TestCompressionCodec empty_name_codec{
        TestCodecBehavior::identity,
        "",
        1U
    };

    check(
        state,
        !CookedAssetArtifactBuildRequest{
            &compressible_asset,
            &empty_name_codec,
            CookedAssetArtifactCompressionPolicy::
                required
        }.is_valid(),
        "Build request rejects an empty codec name");

    TestCompressionCodec zero_version_codec{
        TestCodecBehavior::identity,
        "test.zero-version",
        0U
    };

    check(
        state,
        !CookedAssetArtifactBuildRequest{
            &compressible_asset,
            &zero_version_codec,
            CookedAssetArtifactCompressionPolicy::
                required
        }.is_valid(),
        "Build request rejects codec version zero");

    check(
        state,
        !CookedAssetArtifactBuildRequest{
            &compressible_asset,
            &rle_codec,
            static_cast<
                CookedAssetArtifactCompressionPolicy>(
                    255U)
        }.is_valid(),
        "Build request rejects an unknown compression policy");

    CookedAsset invalid_asset =
        compressible_asset;

    invalid_asset.record.id = {};

    check(
        state,
        !CookedAssetArtifactBuildRequest{
            &invalid_asset,
            nullptr,
            CookedAssetArtifactCompressionPolicy::
                disabled
        }.is_valid(),
        "Build request rejects an invalid cooked asset");

    const CookedAsset original_asset_copy =
        compressible_asset;

    const auto disabled_result =
        build_cooked_asset_artifact(
            disabled_request);

    check(
        state,
        disabled_result.has_value(),
        "Disabled policy builds an artifact");

    CookedAssetArtifact
        uncompressed_artifact{};

    if (disabled_result.has_value())
    {
        uncompressed_artifact =
            disabled_result.value();
    }

    check(
        state,
        disabled_result.has_value() &&
            !uncompressed_artifact.
                is_compressed(),
        "Disabled policy stores an uncompressed artifact");

    check(
        state,
        disabled_result.has_value() &&
            uncompressed_artifact.payload_bytes ==
                compressible_asset.cooked_bytes,
        "Disabled policy preserves cooked bytes");

    check(
        state,
        disabled_result.has_value() &&
            uncompressed_artifact.
                uncompressed_byte_count ==
            static_cast<std::uint64_t>(
                compressible_asset.
                    cooked_bytes.size()),
        "Disabled policy records the decoded byte count");

    check(
        state,
        disabled_result.has_value() &&
            uncompressed_artifact.record ==
                compressible_asset.record &&
            uncompressed_artifact.cooker_name ==
                compressible_asset.cooker_name &&
            uncompressed_artifact.cooker_version ==
                compressible_asset.cooker_version,
        "Artifact build preserves cooked metadata");

    check(
        state,
        cooked_assets_equal(
            compressible_asset,
            original_asset_copy),
        "Artifact build does not mutate the source asset");

    check(
        state,
        !CookedAssetArtifactMaterializationRequest{}.
            is_valid(),
        "Default materialization request is invalid");

    check(
        state,
        CookedAssetArtifactMaterializationRequest{
            &uncompressed_artifact,
            nullptr
        }.is_valid(),
        "Uncompressed artifact requires no codec");

    check(
        state,
        CookedAssetArtifactMaterializationRequest{
            &uncompressed_artifact,
            &identity_codec
        }.is_valid(),
        "Uncompressed artifact permits an unused codec");

    const auto uncompressed_materialization =
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                &uncompressed_artifact,
                nullptr
            });

    check(
        state,
        uncompressed_materialization.has_value(),
        "Uncompressed artifact materializes");

    check(
        state,
        uncompressed_materialization.has_value() &&
            cooked_assets_equal(
                uncompressed_materialization.value(),
                compressible_asset),
        "Uncompressed materialization restores the cooked asset");

    const auto compressible_result =
        build_cooked_asset_artifact(
            CookedAssetArtifactBuildRequest{
                &compressible_asset,
                &rle_codec,
                CookedAssetArtifactCompressionPolicy::
                    when_smaller
            });

    check(
        state,
        compressible_result.has_value(),
        "Conditional policy builds a compressible artifact");

    CookedAssetArtifact compressed_artifact{};

    if (compressible_result.has_value())
    {
        compressed_artifact =
            compressible_result.value();
    }

    check(
        state,
        compressible_result.has_value() &&
            compressed_artifact.is_compressed(),
        "Conditional policy compresses when smaller");

    check(
        state,
        compressible_result.has_value() &&
            compressed_artifact.
                compression_codec_name ==
                    rle_codec.name() &&
            compressed_artifact.
                compression_codec_version ==
                    rle_codec.version(),
        "Compressed artifact records codec identity");

    check(
        state,
        compressible_result.has_value() &&
            compressed_artifact.
                payload_bytes.size() <
            compressible_asset.
                cooked_bytes.size(),
        "Compressed artifact payload is smaller");

    check(
        state,
        CookedAssetArtifactMaterializationRequest{
            &compressed_artifact,
            &rle_codec
        }.is_valid(),
        "Compressed artifact accepts its matching codec");

    check(
        state,
        !CookedAssetArtifactMaterializationRequest{
            &compressed_artifact,
            nullptr
        }.is_valid(),
        "Compressed artifact rejects a missing codec");

    const auto compressed_materialization =
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                &compressed_artifact,
                &rle_codec
            });

    check(
        state,
        compressed_materialization.has_value(),
        "Compressed artifact materializes");

    check(
        state,
        compressed_materialization.has_value() &&
            cooked_assets_equal(
                compressed_materialization.value(),
                compressible_asset),
        "Compressed materialization restores the cooked asset");

    const auto incompressible_result =
        build_cooked_asset_artifact(
            CookedAssetArtifactBuildRequest{
                &incompressible_asset,
                &rle_codec,
                CookedAssetArtifactCompressionPolicy::
                    when_smaller
            });

    check(
        state,
        incompressible_result.has_value(),
        "Conditional policy builds an incompressible artifact");

    check(
        state,
        incompressible_result.has_value() &&
            !incompressible_result.value().
                is_compressed(),
        "Conditional policy falls back when compression expands data");

    check(
        state,
        incompressible_result.has_value() &&
            incompressible_result.value().
                payload_bytes ==
                    incompressible_asset.
                        cooked_bytes,
        "Compression fallback preserves original bytes");

    const auto required_result =
        build_cooked_asset_artifact(
            CookedAssetArtifactBuildRequest{
                &incompressible_asset,
                &rle_codec,
                CookedAssetArtifactCompressionPolicy::
                    required
            });

    check(
        state,
        required_result.has_value() &&
            required_result.value().
                is_compressed(),
        "Required policy keeps expanded compressed data");

    check(
        state,
        required_result.has_value() &&
            required_result.value().
                payload_bytes.size() >
                    incompressible_asset.
                        cooked_bytes.size(),
        "Required policy permits a larger encoded payload");

    const auto required_materialization =
        required_result.has_value()
            ? materialize_cooked_asset_artifact(
                  CookedAssetArtifactMaterializationRequest{
                      &required_result.value(),
                      &rle_codec
                  })
            : oros::foundation::Result<
                  CookedAsset>{
                  oros::foundation::fail(
                      ErrorCode::internal_failure,
                      "Required artifact prerequisite "
                      "failed.")
              };

    check(
        state,
        required_materialization.has_value() &&
            cooked_assets_equal(
                required_materialization.value(),
                incompressible_asset),
        "Required compressed artifact restores the cooked asset");

    const auto identity_required_result =
        build_cooked_asset_artifact(
            CookedAssetArtifactBuildRequest{
                &compressible_asset,
                &identity_codec,
                CookedAssetArtifactCompressionPolicy::
                    required
            });

    check(
        state,
        identity_required_result.has_value() &&
            identity_required_result.value().
                is_compressed(),
        "Required policy stores equal-size codec output");

    check(
        state,
        identity_required_result.has_value() &&
            identity_required_result.value().
                payload_bytes ==
                    compressible_asset.
                        cooked_bytes,
        "Identity codec preserves encoded bytes");

    const auto empty_conditional_result =
        build_cooked_asset_artifact(
            CookedAssetArtifactBuildRequest{
                &empty_asset,
                &rle_codec,
                CookedAssetArtifactCompressionPolicy::
                    when_smaller
            });

    check(
        state,
        empty_conditional_result.has_value() &&
            !empty_conditional_result.value().
                is_compressed(),
        "Conditional policy leaves empty data uncompressed");

    const auto empty_required_result =
        build_cooked_asset_artifact(
            CookedAssetArtifactBuildRequest{
                &empty_asset,
                &rle_codec,
                CookedAssetArtifactCompressionPolicy::
                    required
            });

    check(
        state,
        empty_required_result.has_value() &&
            empty_required_result.value().
                is_compressed(),
        "Required policy represents empty data as compressed");

    check(
        state,
        empty_required_result.has_value() &&
            empty_required_result.value().
                payload_bytes.empty() &&
            empty_required_result.value().
                uncompressed_byte_count == 0ULL,
        "Required empty artifact preserves zero sizes");

    const auto empty_materialization =
        empty_required_result.has_value()
            ? materialize_cooked_asset_artifact(
                  CookedAssetArtifactMaterializationRequest{
                      &empty_required_result.value(),
                      &rle_codec
                  })
            : oros::foundation::Result<
                  CookedAsset>{
                  oros::foundation::fail(
                      ErrorCode::internal_failure,
                      "Empty artifact prerequisite "
                      "failed.")
              };

    check(
        state,
        empty_materialization.has_value() &&
            cooked_assets_equal(
                empty_materialization.value(),
                empty_asset),
        "Empty compressed artifact materializes");

    CookedAsset corrupted_asset =
        compressible_asset;

    corrupted_asset.cooked_bytes[0U] ^=
        std::byte{0x01};

    check_failure(
        state,
        build_cooked_asset_artifact(
            CookedAssetArtifactBuildRequest{
                &corrupted_asset,
                nullptr,
                CookedAssetArtifactCompressionPolicy::
                    disabled
            }),
        ErrorCode::invalid_argument,
        "Build rejects cooked bytes that do not match their hash");

    TestCompressionCodec
        failing_compression_codec{
            TestCodecBehavior::
                fail_compression
        };

    check_failure(
        state,
        build_cooked_asset_artifact(
            CookedAssetArtifactBuildRequest{
                &compressible_asset,
                &failing_compression_codec,
                CookedAssetArtifactCompressionPolicy::
                    required
            }),
        ErrorCode::input_output_failure,
        "Build propagates codec compression failures");

    TestCompressionCodec
        invalid_result_codec{
            TestCodecBehavior::
                invalid_compression_result
        };

    check_failure(
        state,
        build_cooked_asset_artifact(
            CookedAssetArtifactBuildRequest{
                &compressible_asset,
                &invalid_result_codec,
                CookedAssetArtifactCompressionPolicy::
                    required
            }),
        ErrorCode::invalid_state,
        "Build rejects structurally invalid compression results");

    TestCompressionCodec
        wrong_original_count_codec{
            TestCodecBehavior::
                wrong_original_byte_count
        };

    check_failure(
        state,
        build_cooked_asset_artifact(
            CookedAssetArtifactBuildRequest{
                &compressible_asset,
                &wrong_original_count_codec,
                CookedAssetArtifactCompressionPolicy::
                    required
            }),
        ErrorCode::invalid_state,
        "Build rejects an incorrect original byte count");

    TestCompressionCodec
        mutating_compression_codec{
            TestCodecBehavior::
                mutate_during_compression
        };

    check_failure(
        state,
        build_cooked_asset_artifact(
            CookedAssetArtifactBuildRequest{
                &compressible_asset,
                &mutating_compression_codec,
                CookedAssetArtifactCompressionPolicy::
                    required
            }),
        ErrorCode::invalid_state,
        "Build detects codec metadata mutation");

    TestCompressionCodec
        throwing_compression_codec{
            TestCodecBehavior::
                throw_during_compression
        };

    check_failure(
        state,
        build_cooked_asset_artifact(
            CookedAssetArtifactBuildRequest{
                &compressible_asset,
                &throwing_compression_codec,
                CookedAssetArtifactCompressionPolicy::
                    required
            }),
        ErrorCode::internal_failure,
        "Build converts unexpected codec exceptions");

    CookedAssetArtifact
        corrupted_uncompressed_artifact =
            uncompressed_artifact;

    corrupted_uncompressed_artifact.
        payload_bytes[0U] ^=
            std::byte{0x01};

    check_failure(
        state,
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                &corrupted_uncompressed_artifact,
                nullptr
            }),
        ErrorCode::invalid_argument,
        "Materialization rejects corrupted uncompressed bytes");

    CookedAssetArtifact
        invalid_uncompressed_size =
            uncompressed_artifact;

    ++invalid_uncompressed_size.
        uncompressed_byte_count;

    check_failure(
        state,
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                &invalid_uncompressed_size,
                nullptr
            }),
        ErrorCode::invalid_argument,
        "Materialization rejects inconsistent uncompressed metadata");

    TestCompressionCodec wrong_name_codec{
        TestCodecBehavior::identity,
        "test.other",
        7U
    };

    check_failure(
        state,
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                identity_required_result.has_value()
                    ? &identity_required_result.value()
                    : nullptr,
                &wrong_name_codec
            }),
        ErrorCode::invalid_argument,
        "Materialization rejects a codec name mismatch");

    TestCompressionCodec wrong_version_codec{
        TestCodecBehavior::identity,
        "test.identity",
        8U
    };

    check_failure(
        state,
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                identity_required_result.has_value()
                    ? &identity_required_result.value()
                    : nullptr,
                &wrong_version_codec
            }),
        ErrorCode::invalid_argument,
        "Materialization rejects a codec version mismatch");

    TestCompressionCodec
        failing_decompression_codec{
            TestCodecBehavior::
                fail_decompression
        };

    check_failure(
        state,
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                identity_required_result.has_value()
                    ? &identity_required_result.value()
                    : nullptr,
                &failing_decompression_codec
            }),
        ErrorCode::input_output_failure,
        "Materialization propagates decompression failures");

    TestCompressionCodec
        wrong_decoded_count_codec{
            TestCodecBehavior::
                wrong_decoded_byte_count
        };

    check_failure(
        state,
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                identity_required_result.has_value()
                    ? &identity_required_result.value()
                    : nullptr,
                &wrong_decoded_count_codec
            }),
        ErrorCode::invalid_state,
        "Materialization rejects an unexpected decoded size");

    TestCompressionCodec
        mutating_decompression_codec{
            TestCodecBehavior::
                mutate_during_decompression
        };

    check_failure(
        state,
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                identity_required_result.has_value()
                    ? &identity_required_result.value()
                    : nullptr,
                &mutating_decompression_codec
            }),
        ErrorCode::invalid_state,
        "Materialization detects codec metadata mutation");

    TestCompressionCodec
        throwing_decompression_codec{
            TestCodecBehavior::
                throw_during_decompression
        };

    check_failure(
        state,
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                identity_required_result.has_value()
                    ? &identity_required_result.value()
                    : nullptr,
                &throwing_decompression_codec
            }),
        ErrorCode::internal_failure,
        "Materialization converts unexpected codec exceptions");

    CookedAssetArtifact
        corrupted_compressed_artifact{};

    if (identity_required_result.has_value())
    {
        corrupted_compressed_artifact =
            identity_required_result.value();

        corrupted_compressed_artifact.
            payload_bytes[0U] ^=
                std::byte{0x01};
    }

    check_failure(
        state,
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                &corrupted_compressed_artifact,
                &identity_codec
            }),
        ErrorCode::invalid_argument,
        "Materialization rejects corrupted decoded content");

    CookedAssetArtifact
        malformed_rle_artifact =
            compressed_artifact;

    malformed_rle_artifact.payload_bytes = {
        std::byte{0x01}
    };

    check_failure(
        state,
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                &malformed_rle_artifact,
                &rle_codec
            }),
        ErrorCode::invalid_argument,
        "Materialization rejects malformed compressed data");

    const auto uncompressed_with_codec =
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                &uncompressed_artifact,
                &throwing_decompression_codec
            });

    check(
        state,
        uncompressed_with_codec.has_value() &&
            cooked_assets_equal(
                uncompressed_with_codec.value(),
                compressible_asset),
        "Uncompressed materialization does not invoke an unused codec");

    check_failure(
        state,
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{}),
        ErrorCode::invalid_argument,
        "Materialization rejects a default request");

    CookedAssetArtifact invalid_artifact{};

    check_failure(
        state,
        materialize_cooked_asset_artifact(
            CookedAssetArtifactMaterializationRequest{
                &invalid_artifact,
                nullptr
            }),
        ErrorCode::invalid_argument,
        "Materialization rejects an invalid artifact");

    std::cout
        << "\nCooked asset artifact pipeline "
           "test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return
        state.failures == 0
            ? 0
            : 1;
}