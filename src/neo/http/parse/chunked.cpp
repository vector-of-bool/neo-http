#include "./chunked.hpp"

#include "./common.hpp"

#include <charconv>

using namespace neo;

http::chunk_head http::chunk_head::parse(const_buffer cb) noexcept {
    static chunk_head invalid_ret = {size_t(-1), const_buffer(), const_buffer()};

    std::size_t chunk_size = 0;
    const auto  num_begin  = reinterpret_cast<const char*>(cb.data());
    const auto  conv_res   = std::from_chars(num_begin, num_begin + cb.size(), chunk_size, 16);

    if (conv_res.ec != std::errc{}) {
        return invalid_ret;
    }

    cb += (conv_res.ptr - num_begin);
    auto ext_full = cb;

    while (!cb.empty() && !begins_with_crlf(cb)) {
        cb += 1;
    }

    if (!begins_with_crlf(cb)) {
        return invalid_ret;
    }
    cb += 2;

    return {chunk_size, ext_full.first(cb.data() - ext_full.data() - 2), cb};
}
