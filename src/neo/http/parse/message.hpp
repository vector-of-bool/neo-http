#pragma once

#include "./header.hpp"

#include <neo/const_buffer.hpp>

namespace neo::http {

template <typename Derived, typename StartLine>
struct message_head {
    using start_line_type = StartLine;

    start_line_type  start_line;
    header_lines_buf headers;
    const_buffer     parse_tail;

    constexpr bool valid() const noexcept { return start_line.valid() && headers.valid(); }

    constexpr static Derived parse(const_buffer buf) noexcept {
        auto sl = start_line_type::parse(buf);
        if (!sl.valid()) {
            return {};
        }
        auto hl = header_lines_buf::parse(sl.parse_tail);
        return {sl, hl, hl.parse_tail};
    }
};

}  // namespace neo::http