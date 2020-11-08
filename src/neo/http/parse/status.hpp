#pragma once

#include <neo/http/parse/common.hpp>
#include <neo/http/parse/version.hpp>

#include <neo/const_buffer.hpp>

#include <string_view>

namespace neo::http {

struct status_line {
    version          http_version = version::invalid;
    int              status       = 0;
    std::string_view phrase_view;

    const_buffer parse_tail;

    static status_line parse(neo::const_buffer buf) noexcept;

    constexpr bool valid() const noexcept {
        return http_version != version::invalid && status >= 100 && status <= 999;
    }

    mutable_buffer write(mutable_buffer) const noexcept;

    constexpr std::size_t required_write_size() const noexcept {
        return version_buf_size + 1    // +1 for SP
            + 3 + 1                    // +3 for code, +1 for SP
            + phrase_view.size() + 2;  // +2 for CRLF
    }
};

std::string_view default_phrase(int code) noexcept;

}  // namespace neo::http
