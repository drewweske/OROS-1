#include "oros/assets/content_hash.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <string_view>

namespace oros::assets
{
    namespace
    {
        inline constexpr std::size_t
            sha256_block_byte_count{64U};

        inline constexpr std::size_t
            sha256_word_count{64U};

        inline constexpr std::array<
            std::uint32_t,
            sha256_word_count
        > round_constants{
            0x428A2F98U, 0x71374491U,
            0xB5C0FBCFU, 0xE9B5DBA5U,
            0x3956C25BU, 0x59F111F1U,
            0x923F82A4U, 0xAB1C5ED5U,
            0xD807AA98U, 0x12835B01U,
            0x243185BEU, 0x550C7DC3U,
            0x72BE5D74U, 0x80DEB1FEU,
            0x9BDC06A7U, 0xC19BF174U,
            0xE49B69C1U, 0xEFBE4786U,
            0x0FC19DC6U, 0x240CA1CCU,
            0x2DE92C6FU, 0x4A7484AAU,
            0x5CB0A9DCU, 0x76F988DAU,
            0x983E5152U, 0xA831C66DU,
            0xB00327C8U, 0xBF597FC7U,
            0xC6E00BF3U, 0xD5A79147U,
            0x06CA6351U, 0x14292967U,
            0x27B70A85U, 0x2E1B2138U,
            0x4D2C6DFCU, 0x53380D13U,
            0x650A7354U, 0x766A0ABBU,
            0x81C2C92EU, 0x92722C85U,
            0xA2BFE8A1U, 0xA81A664BU,
            0xC24B8B70U, 0xC76C51A3U,
            0xD192E819U, 0xD6990624U,
            0xF40E3585U, 0x106AA070U,
            0x19A4C116U, 0x1E376C08U,
            0x2748774CU, 0x34B0BCB5U,
            0x391C0CB3U, 0x4ED8AA4AU,
            0x5B9CCA4FU, 0x682E6FF3U,
            0x748F82EEU, 0x78A5636FU,
            0x84C87814U, 0x8CC70208U,
            0x90BEFFFAU, 0xA4506CEBU,
            0xBEF9A3F7U, 0xC67178F2U
        };

        inline constexpr std::array<
            std::uint32_t,
            8U
        > initial_state{
            0x6A09E667U,
            0xBB67AE85U,
            0x3C6EF372U,
            0xA54FF53AU,
            0x510E527FU,
            0x9B05688CU,
            0x1F83D9ABU,
            0x5BE0CD19U
        };

        inline constexpr char hex_digits[]{
            "0123456789abcdef"
        };

        [[nodiscard]]
        std::uint32_t load_big_endian_word(
            const std::byte* const bytes) noexcept
        {
            return
                (std::to_integer<std::uint32_t>(
                    bytes[0]) << 24U) |
                (std::to_integer<std::uint32_t>(
                    bytes[1]) << 16U) |
                (std::to_integer<std::uint32_t>(
                    bytes[2]) << 8U) |
                std::to_integer<std::uint32_t>(
                    bytes[3]);
        }

        void process_block(
            std::array<std::uint32_t, 8U>& state,
            const std::byte* const block) noexcept
        {
            std::array<
                std::uint32_t,
                sha256_word_count
            > schedule{};

            for (std::size_t index = 0U;
                 index < 16U;
                 ++index)
            {
                schedule[index] =
                    load_big_endian_word(
                        block + index * 4U);
            }

            for (std::size_t index = 16U;
                 index < sha256_word_count;
                 ++index)
            {
                const std::uint32_t first =
                    std::rotr(
                        schedule[index - 15U],
                        7) ^
                    std::rotr(
                        schedule[index - 15U],
                        18) ^
                    (schedule[index - 15U] >>
                        3U);

                const std::uint32_t second =
                    std::rotr(
                        schedule[index - 2U],
                        17) ^
                    std::rotr(
                        schedule[index - 2U],
                        19) ^
                    (schedule[index - 2U] >>
                        10U);

                schedule[index] =
                    schedule[index - 16U] +
                    first +
                    schedule[index - 7U] +
                    second;
            }

            std::uint32_t a{state[0]};
            std::uint32_t b{state[1]};
            std::uint32_t c{state[2]};
            std::uint32_t d{state[3]};
            std::uint32_t e{state[4]};
            std::uint32_t f{state[5]};
            std::uint32_t g{state[6]};
            std::uint32_t h{state[7]};

            for (std::size_t index = 0U;
                 index < sha256_word_count;
                 ++index)
            {
                const std::uint32_t large_first =
                    std::rotr(e, 6) ^
                    std::rotr(e, 11) ^
                    std::rotr(e, 25);

                const std::uint32_t choose =
                    (e & f) ^
                    (~e & g);

                const std::uint32_t temporary_first =
                    h +
                    large_first +
                    choose +
                    round_constants[index] +
                    schedule[index];

                const std::uint32_t large_second =
                    std::rotr(a, 2) ^
                    std::rotr(a, 13) ^
                    std::rotr(a, 22);

                const std::uint32_t majority =
                    (a & b) ^
                    (a & c) ^
                    (b & c);

                const std::uint32_t temporary_second =
                    large_second +
                    majority;

                h = g;
                g = f;
                f = e;
                e = d + temporary_first;
                d = c;
                c = b;
                b = a;
                a =
                    temporary_first +
                    temporary_second;
            }

            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
            state[4] += e;
            state[5] += f;
            state[6] += g;
            state[7] += h;
        }

        void write_big_endian_word(
            const std::uint32_t word,
            std::byte* const output) noexcept
        {
            output[0] =
                std::byte{
                    static_cast<unsigned char>(
                        (word >> 24U) &
                        0xFFU)
                };

            output[1] =
                std::byte{
                    static_cast<unsigned char>(
                        (word >> 16U) &
                        0xFFU)
                };

            output[2] =
                std::byte{
                    static_cast<unsigned char>(
                        (word >> 8U) &
                        0xFFU)
                };

            output[3] =
                std::byte{
                    static_cast<unsigned char>(
                        word &
                        0xFFU)
                };
        }

        [[nodiscard]]
        bool decode_hex_digit(
            const char character,
            unsigned char& value) noexcept
        {
            if (character >= '0' &&
                character <= '9')
            {
                value =
                    static_cast<unsigned char>(
                        character - '0');

                return true;
            }

            if (character >= 'a' &&
                character <= 'f')
            {
                value =
                    static_cast<unsigned char>(
                        character - 'a' +
                        10);

                return true;
            }

            if (character >= 'A' &&
                character <= 'F')
            {
                value =
                    static_cast<unsigned char>(
                        character - 'A' +
                        10);

                return true;
            }

            return false;
        }
    }

    std::size_t
    ContentHashHash::operator()(
        const ContentHash& hash) const noexcept
    {
        std::uint64_t value{
            14695981039346656037ULL
        };

        for (const std::byte byte : hash.bytes)
        {
            value ^=
                static_cast<std::uint64_t>(
                    std::to_integer<
                        unsigned int>(byte));

            value *=
                1099511628211ULL;
        }

        if constexpr (
            sizeof(std::size_t) >=
            sizeof(std::uint64_t))
        {
            return static_cast<std::size_t>(
                value);
        }
        else
        {
            return static_cast<std::size_t>(
                value ^
                (value >> 32U));
        }
    }

    ContentHash hash_bytes(
        const std::span<
            const std::byte
        > bytes) noexcept
    {
        std::array<std::uint32_t, 8U> state =
            initial_state;

        const std::size_t complete_blocks =
            bytes.size() /
            sha256_block_byte_count;

        for (std::size_t block_index = 0U;
             block_index < complete_blocks;
             ++block_index)
        {
            process_block(
                state,
                bytes.data() +
                    block_index *
                    sha256_block_byte_count);
        }

        const std::size_t consumed_bytes =
            complete_blocks *
            sha256_block_byte_count;

        const std::size_t remaining_bytes =
            bytes.size() -
            consumed_bytes;

        std::array<
            std::byte,
            sha256_block_byte_count * 2U
        > tail{};

        for (std::size_t index = 0U;
             index < remaining_bytes;
             ++index)
        {
            tail[index] =
                bytes[consumed_bytes + index];
        }

        tail[remaining_bytes] =
            std::byte{0x80U};

        const std::size_t tail_block_count =
            remaining_bytes < 56U ?
                1U :
                2U;

        const std::size_t length_offset =
            tail_block_count *
                sha256_block_byte_count -
            8U;

        const std::uint64_t bit_length =
            static_cast<std::uint64_t>(
                bytes.size()) *
            8ULL;

        for (std::size_t index = 0U;
             index < 8U;
             ++index)
        {
            const unsigned shift =
                static_cast<unsigned>(
                    (7U - index) *
                    8U);

            tail[length_offset + index] =
                std::byte{
                    static_cast<unsigned char>(
                        (bit_length >> shift) &
                        0xFFULL)
                };
        }

        for (std::size_t block_index = 0U;
             block_index < tail_block_count;
             ++block_index)
        {
            process_block(
                state,
                tail.data() +
                    block_index *
                    sha256_block_byte_count);
        }

        ContentHash hash{};

        for (std::size_t index = 0U;
             index < state.size();
             ++index)
        {
            write_big_endian_word(
                state[index],
                hash.bytes.data() +
                    index * 4U);
        }

        return hash;
    }

    ContentHash hash_text(
        const std::string_view text) noexcept
    {
        const std::span<const char> characters{
            text.data(),
            text.size()
        };

        return hash_bytes(
            std::as_bytes(characters));
    }

    std::string to_string(
        const ContentHash& hash)
    {
        std::string text(
            content_hash_text_length,
            '0');

        for (std::size_t index = 0U;
             index < hash.bytes.size();
             ++index)
        {
            const unsigned int value =
                std::to_integer<unsigned int>(
                    hash.bytes[index]);

            text[index * 2U] =
                hex_digits[
                    static_cast<std::size_t>(
                        (value >> 4U) &
                        0x0FU)];

            text[index * 2U + 1U] =
                hex_digits[
                    static_cast<std::size_t>(
                        value &
                        0x0FU)];
        }

        return text;
    }

    foundation::Result<ContentHash>
    parse_content_hash(
        const std::string_view text)
    {
        if (text.size() !=
            content_hash_text_length)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Content hash text must contain exactly "
                "64 hexadecimal digits.");
        }

        ContentHash hash{};

        for (std::size_t index = 0U;
             index < hash.bytes.size();
             ++index)
        {
            unsigned char high{};
            unsigned char low{};

            const bool high_valid =
                decode_hex_digit(
                    text[index * 2U],
                    high);

            const bool low_valid =
                decode_hex_digit(
                    text[index * 2U + 1U],
                    low);

            if (!high_valid ||
                !low_valid)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Content hash text contains a "
                    "non-hexadecimal digit.");
            }

            hash.bytes[index] =
                std::byte{
                    static_cast<unsigned char>(
                        static_cast<unsigned int>(
                            high) *
                            16U +
                        static_cast<unsigned int>(
                            low))
                };
        }

        return hash;
    }
}