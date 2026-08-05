#include "oros/assets/cooked_asset_cache.hpp"

#include "oros/assets/content_hash.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
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

    [[nodiscard]]
    oros::assets::CookedAsset
    make_cooked_asset(
        const oros::assets::AssetId id,
        std::string source_path,
        std::vector<std::byte> cooked_bytes)
    {
        using namespace oros::assets;

        CookedAsset asset{};

        asset.record.id = id;

        asset.record.source_path =
            std::move(
                source_path);

        asset.record.importer_name =
            "oros.cache_importer";

        asset.record.importer_version =
            3U;

        asset.record.schema_version =
            7U;

        asset.record.source_hash =
            hash_text(
                asset.record.source_path);

        asset.record.cooked_hash =
            hash_bytes(
                std::span<const std::byte>{
                    cooked_bytes
                });

        asset.cooker_name =
            "oros.cache_cooker";

        asset.cooker_version =
            5U;

        asset.cooked_bytes =
            std::move(
                cooked_bytes);

        return asset;
    }

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::
                CookedAssetCache>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::
                CookedAssetCache>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            oros::assets::
                CookedAssetCache>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            oros::assets::
                CookedAssetCache>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};
    CookedAssetCache cache{};

    const ContentHash missing_hash =
        hash_text(
            "missing cooked payload");

    check(
        state,
        cache.empty(),
        "New cooked asset cache is empty");

    check(
        state,
        cache.size() == 0U,
        "New cooked asset cache has zero entries");

    check(
        state,
        cache.total_byte_count() == 0U,
        "New cooked asset cache has zero bytes");

    check(
        state,
        cache.find(
            missing_hash) ==
            nullptr,
        "Missing content lookup returns null");

    check(
        state,
        !cache.contains(
            missing_hash),
        "Missing content is not contained");

    const Result<bool>
        invalid_store =
            cache.store(
                CookedAsset{});

    check(
        state,
        !invalid_store.has_value(),
        "Cache rejects an invalid cooked asset");

    check(
        state,
        !invalid_store.has_value() &&
            invalid_store.error().code ==
                ErrorCode::invalid_argument,
        "Invalid cooked asset reports invalid_argument");

    check(
        state,
        cache.empty(),
        "Rejected invalid asset does not mutate the cache");

    CookedAsset first_asset =
        make_cooked_asset(
            AssetId{
                10ULL,
                1ULL
            },
            "textures/cache/first.asset",
            {
                std::byte{0x10},
                std::byte{0x20},
                std::byte{0x30},
                std::byte{0x40}
            });

    check(
        state,
        first_asset.is_valid(),
        "First cooked asset is valid");

    check(
        state,
        first_asset.record.cooked_hash.
            has_value(),
        "First cooked asset has a content hash");

    const ContentHash first_hash =
        first_asset.record.cooked_hash.
            value();

    CookedAsset mismatched_asset =
        first_asset;

    mismatched_asset.cooked_bytes[0] =
        std::byte{0xFF};

    check(
        state,
        mismatched_asset.is_valid(),
        "Hash-mismatched metadata remains structurally valid");

    const Result<bool>
        mismatched_store =
            cache.store(
                mismatched_asset);

    check(
        state,
        !mismatched_store.has_value(),
        "Cache rejects payload data that does not match its hash");

    check(
        state,
        !mismatched_store.has_value() &&
            mismatched_store.error().code ==
                ErrorCode::invalid_argument,
        "Payload hash mismatch reports invalid_argument");

    check(
        state,
        cache.empty(),
        "Rejected hash mismatch does not mutate the cache");

    const Result<bool>
        first_store =
            cache.store(
                first_asset);

    check(
        state,
        first_store.has_value(),
        "Cache accepts the first cooked asset");

    check(
        state,
        first_store.has_value() &&
            first_store.value(),
        "First cooked asset reports a new insertion");

    check(
        state,
        !cache.empty(),
        "Cache is non-empty after insertion");

    check(
        state,
        cache.size() == 1U,
        "First insertion increases cache size");

    check(
        state,
        cache.total_byte_count() == 4U,
        "First insertion updates the total byte count");

    check(
        state,
        cache.contains(
            first_hash),
        "Cache contains the first content hash");

    const std::vector<std::byte>*
        first_lookup =
            cache.find(
                first_hash);

    check(
        state,
        first_lookup != nullptr,
        "First content hash lookup succeeds");

    check(
        state,
        first_lookup != nullptr &&
            *first_lookup ==
                first_asset.cooked_bytes,
        "First lookup preserves every cooked byte");

    const std::vector<std::byte>* const
        first_lookup_address =
            first_lookup;

    const Result<bool>
        duplicate_store =
            cache.store(
                first_asset);

    check(
        state,
        duplicate_store.has_value(),
        "Cache accepts a duplicate store operation");

    check(
        state,
        duplicate_store.has_value() &&
            !duplicate_store.value(),
        "Identical content reports deduplication");

    check(
        state,
        cache.size() == 1U,
        "Duplicate storage preserves cache size");

    check(
        state,
        cache.total_byte_count() == 4U,
        "Duplicate storage preserves total bytes");

    check(
        state,
        cache.find(
            first_hash) ==
            first_lookup_address,
        "Duplicate storage preserves the original entry");

    CookedAsset same_content_asset =
        make_cooked_asset(
            AssetId{
                10ULL,
                2ULL
            },
            "textures/cache/same-content.asset",
            first_asset.cooked_bytes);

    check(
        state,
        same_content_asset.is_valid(),
        "Second asset with identical content is valid");

    check(
        state,
        same_content_asset.record.id !=
            first_asset.record.id,
        "Identical content can belong to another asset identity");

    check(
        state,
        same_content_asset.record.cooked_hash ==
            first_asset.record.cooked_hash,
        "Identical payloads produce an identical content hash");

    const Result<bool>
        same_content_store =
            cache.store(
                same_content_asset);

    check(
        state,
        same_content_store.has_value(),
        "Cache accepts identical content from another asset");

    check(
        state,
        same_content_store.has_value() &&
            !same_content_store.value(),
        "Identical content from another asset is deduplicated");

    check(
        state,
        cache.size() == 1U,
        "Cross-asset deduplication preserves cache size");

    check(
        state,
        cache.total_byte_count() == 4U,
        "Cross-asset deduplication preserves total bytes");

    CookedAsset second_asset =
        make_cooked_asset(
            AssetId{
                10ULL,
                3ULL
            },
            "textures/cache/second.asset",
            {
                std::byte{0xAA},
                std::byte{0xBB},
                std::byte{0xCC}
            });

    const ContentHash second_hash =
        second_asset.record.cooked_hash.
            value();

    check(
        state,
        second_hash != first_hash,
        "Different payload produces a different content hash");

    const Result<bool>
        second_store =
            cache.store(
                second_asset);

    check(
        state,
        second_store.has_value() &&
            second_store.value(),
        "Cache inserts a distinct cooked payload");

    check(
        state,
        cache.size() == 2U,
        "Distinct insertion increases cache size");

    check(
        state,
        cache.total_byte_count() == 7U,
        "Distinct insertion adds its byte count");

    check(
        state,
        cache.contains(
            first_hash) &&
            cache.contains(
                second_hash),
        "Cache contains both distinct payloads");

    const std::vector<std::byte>*
        second_lookup =
            cache.find(
                second_hash);

    check(
        state,
        second_lookup != nullptr &&
            *second_lookup ==
                second_asset.cooked_bytes,
        "Second lookup preserves every cooked byte");

    CookedAsset empty_asset =
        make_cooked_asset(
            AssetId{
                10ULL,
                4ULL
            },
            "textures/cache/empty.asset",
            {});

    check(
        state,
        empty_asset.is_valid(),
        "Empty cooked payload forms a valid asset");

    check(
        state,
        !empty_asset.has_cooked_data(),
        "Empty cooked payload reports no data");

    const ContentHash empty_hash =
        empty_asset.record.cooked_hash.
            value();

    const Result<bool>
        empty_store =
            cache.store(
                empty_asset);

    check(
        state,
        empty_store.has_value() &&
            empty_store.value(),
        "Cache stores an empty cooked payload");

    check(
        state,
        cache.size() == 3U,
        "Empty payload increases the entry count");

    check(
        state,
        cache.total_byte_count() == 7U,
        "Empty payload does not increase total bytes");

    check(
        state,
        cache.contains(
            empty_hash),
        "Cache contains the empty payload hash");

    const std::vector<std::byte>*
        empty_lookup =
            cache.find(
                empty_hash);

    check(
        state,
        empty_lookup != nullptr,
        "Empty payload lookup succeeds");

    check(
        state,
        empty_lookup != nullptr &&
            empty_lookup->empty(),
        "Empty payload lookup returns an empty vector");

    const Result<bool>
        duplicate_empty_store =
            cache.store(
                empty_asset);

    check(
        state,
        duplicate_empty_store.
                has_value() &&
            !duplicate_empty_store.value(),
        "Duplicate empty payload is deduplicated");

    check(
        state,
        cache.size() == 3U &&
            cache.total_byte_count() == 7U,
        "Duplicate empty payload preserves cache accounting");

    const Status missing_erase =
        cache.erase(
            missing_hash);

    check(
        state,
        !missing_erase.has_value(),
        "Cache rejects erasing a missing hash");

    check(
        state,
        !missing_erase.has_value() &&
            missing_erase.error().code ==
                ErrorCode::not_found,
        "Missing erase reports not_found");

    check(
        state,
        cache.size() == 3U &&
            cache.total_byte_count() == 7U,
        "Rejected erase preserves cache accounting");

    const Status second_erase =
        cache.erase(
            second_hash);

    check(
        state,
        second_erase.has_value(),
        "Cache erases a stored payload");

    check(
        state,
        !cache.contains(
            second_hash),
        "Erased payload is no longer contained");

    check(
        state,
        cache.find(
            second_hash) ==
            nullptr,
        "Erased payload lookup returns null");

    check(
        state,
        cache.size() == 2U,
        "Erase decreases the entry count");

    check(
        state,
        cache.total_byte_count() == 4U,
        "Erase subtracts the payload byte count");

    check(
        state,
        cache.contains(
            first_hash) &&
            cache.contains(
                empty_hash),
        "Erase preserves unrelated cached payloads");

    const Status repeated_erase =
        cache.erase(
            second_hash);

    check(
        state,
        !repeated_erase.has_value(),
        "Cache rejects repeated erase");

    check(
        state,
        !repeated_erase.has_value() &&
            repeated_erase.error().code ==
                ErrorCode::not_found,
        "Repeated erase reports not_found");

    const Status empty_erase =
        cache.erase(
            empty_hash);

    check(
        state,
        empty_erase.has_value(),
        "Cache erases an empty payload");

    check(
        state,
        cache.size() == 1U,
        "Erasing an empty payload decreases entry count");

    check(
        state,
        cache.total_byte_count() == 4U,
        "Erasing an empty payload preserves total bytes");

    cache.clear();

    check(
        state,
        cache.empty(),
        "Clear removes every cached payload");

    check(
        state,
        cache.size() == 0U,
        "Clear resets cache size");

    check(
        state,
        cache.total_byte_count() == 0U,
        "Clear resets total byte count");

    check(
        state,
        cache.find(
            first_hash) ==
            nullptr,
        "Clear removes content lookup entries");

    const Result<bool>
        store_after_clear =
            cache.store(
                second_asset);

    check(
        state,
        store_after_clear.has_value() &&
            store_after_clear.value(),
        "Cache can be reused after clear");

    check(
        state,
        cache.size() == 1U &&
            cache.total_byte_count() == 3U,
        "Reused cache restores correct accounting");

    CookedAssetCache move_constructed{
        std::move(
            cache)
    };

    check(
        state,
        move_constructed.size() == 1U,
        "Move construction transfers cache size");

    check(
        state,
        move_constructed.total_byte_count() ==
            3U,
        "Move construction transfers total bytes");

    check(
        state,
        move_constructed.contains(
            second_hash),
        "Move construction transfers cached content");

    check(
        state,
        cache.empty(),
        "Move construction empties the source cache");

    check(
        state,
        cache.size() == 0U &&
            cache.total_byte_count() == 0U,
        "Moved-from cache resets its accounting");

    const Result<bool>
        moved_from_store =
            cache.store(
                first_asset);

    check(
        state,
        moved_from_store.has_value() &&
            moved_from_store.value(),
        "Moved-from cache can be reused");

    check(
        state,
        cache.contains(
            first_hash),
        "Reused moved-from cache stores content");

    CookedAssetCache
        move_assignment_destination{};

    const Result<bool>
        destination_store =
            move_assignment_destination.
                store(
                    empty_asset);

    check(
        state,
        destination_store.has_value() &&
            destination_store.value(),
        "Move-assignment destination begins with content");

    check(
        state,
        move_assignment_destination.
            contains(
                empty_hash),
        "Move-assignment destination contains its original entry");

    move_assignment_destination =
        std::move(
            move_constructed);

    check(
        state,
        move_assignment_destination.size() ==
            1U,
        "Move assignment transfers cache size");

    check(
        state,
        move_assignment_destination.
            total_byte_count() == 3U,
        "Move assignment transfers total bytes");

    check(
        state,
        move_assignment_destination.
            contains(
                second_hash),
        "Move assignment transfers cached content");

    check(
        state,
        !move_assignment_destination.
            contains(
                empty_hash),
        "Move assignment discards previous destination content");

    check(
        state,
        move_constructed.empty(),
        "Move assignment empties the source cache");

    check(
        state,
        move_constructed.size() == 0U &&
            move_constructed.
                total_byte_count() == 0U,
        "Move-assigned source resets its accounting");

    const std::size_t
        size_before_self_move =
            move_assignment_destination.
                size();

    const std::size_t
        bytes_before_self_move =
            move_assignment_destination.
                total_byte_count();

    const std::vector<std::byte>* const
        payload_before_self_move =
            move_assignment_destination.
                find(
                    second_hash);

    move_assignment_destination =
        std::move(
            move_assignment_destination);

    check(
        state,
        move_assignment_destination.size() ==
            size_before_self_move,
        "Self move assignment preserves cache size");

    check(
        state,
        move_assignment_destination.
            total_byte_count() ==
            bytes_before_self_move,
        "Self move assignment preserves total bytes");

    check(
        state,
        move_assignment_destination.find(
            second_hash) ==
            payload_before_self_move,
        "Self move assignment preserves cached content");

    const Status final_erase =
        move_assignment_destination.erase(
            second_hash);

    check(
        state,
        final_erase.has_value(),
        "Moved cache can erase transferred content");

    check(
        state,
        move_assignment_destination.empty(),
        "Moved cache becomes empty after final erase");

    check(
        state,
        move_assignment_destination.
            total_byte_count() == 0U,
        "Final erase resets transferred byte accounting");

    std::cout
        << "\nCooked asset cache test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}