#pragma once

#include <neo/const_buffer.hpp>

namespace neo::http {

constexpr bool is_crlf(const_buffer buf) {
    return buf.size() == 2 && buf[0] == std::byte{'\r'} && buf[1] == std::byte{'\n'};
}

constexpr bool begins_with_crlf(const_buffer buf) {
    return buf.size() >= 2 && is_crlf(buf.first(2));
}

constexpr std::ptrdiff_t find_crlf(const const_buffer buf) {
    for (auto b2 = buf; b2; b2 += 1) {
        if (begins_with_crlf(b2)) {
            return b2.data() - buf.data();
        }
    }
    return -1;
}

}  // namespace neo::http