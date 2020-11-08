#include <neo/http/parse/version.hpp>

#include <neo/assert.hpp>
#include <neo/buffer_algorithm.hpp>

using namespace std::string_view_literals;

namespace {

bool is_valid_version(neo::http::version ver) {
    return ver == neo::http::version::v1_0 || ver == neo::http::version::v1_1;
}

}  // namespace

neo::http::version neo::http::parse_version(neo::const_buffer buf) noexcept {
    static constexpr auto HTTP_VER_PREFIX = "HTTP/1."sv;
    static_assert(HTTP_VER_PREFIX.size() + 1 == version_buf_size);
    if (buf.size() != HTTP_VER_PREFIX.size() + 1) {
        return version::invalid;
    }

    auto [prefix, ver_digit] = buf.split(HTTP_VER_PREFIX.size());
    if (!prefix.equals_string(HTTP_VER_PREFIX)) {
        return version::invalid;
    }

    if (ver_digit[0] == std::byte{'0'}) {
        return version::v1_0;
    } else if (ver_digit[0] == std::byte{'1'}) {
        return version::v1_1;
    } else {
        return version::invalid;
    }
}

neo::const_buffer neo::http::version_buf(version ver) noexcept {
    static constexpr auto v1_0_str = "HTTP/1.0"sv;
    static constexpr auto v1_1_str = "HTTP/1.1"sv;
    neo_assert(expects,
               is_valid_version(ver),
               "Invalid version given to neo::http::version_buf",
               int(ver));
    switch (ver) {
    case version::v1_0:
        return const_buffer(v1_0_str);
    case version::v1_1:
        return const_buffer(v1_1_str);
    default:
        neo::unreachable();
        std::terminate();
    }
}

neo::mutable_buffer neo::http::write_version(mutable_buffer mb, version ver) noexcept {
    neo_assert(expects,
               is_valid_version(ver),
               "Invalid version given to neo::http::write_version",
               int(ver));
    auto ver_buf = version_buf(ver);
    neo_assert(invariant,
               ver_buf.size() <= version_buf_size,
               "Version buffer came out too large?",
               ver_buf.size(),
               version_buf_size);
    neo_assert(expects,
               mb.size() >= ver_buf.size(),
               "Write-buffer is too small for neo::http::write_version",
               mb.size(),
               ver_buf.size());
    return mb + neo::buffer_copy(mb, ver_buf);
}
