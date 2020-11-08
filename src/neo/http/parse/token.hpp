#pragma once

#include <neo/const_buffer.hpp>

#include <string_view>

namespace neo::http {

struct token {
    std::string_view view;
    const_buffer     parse_tail;

    static token parse_next(const_buffer) noexcept;

    constexpr bool valid() const noexcept { return !view.empty(); }
};

}  // namespace neo::http
