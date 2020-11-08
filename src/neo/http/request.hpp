#pragma once

#include "./parse/request.hpp"

#include <neo/buffer_algorithm/copy.hpp>
#include <neo/buffer_algorithm/encode.hpp>
#include <neo/buffer_range.hpp>
#include <neo/buffers_cat.hpp>
#include <neo/pp.hpp>
#include <neo/switch_coro.hpp>

#include <neo/concepts.hpp>

namespace neo::http {

template <buffer_output Out, typename Headers, buffer_input Body>
std::size_t
write_request(Out&& out_, const request_line& req_line, Headers&& headers, Body&& body) {
    using namespace literals;
    auto&& out       = ensure_buffer_sink(out_);
    auto   n_written = req_line.write(out);

    for (const auto& [key, value] : headers) {
        auto&& key_buf = as_buffer(key);
        auto&& val_buf = as_buffer(value);
        n_written += buffer_copy(out, buffers_cat(key_buf, ": "_buf, val_buf, "\r\n"_buf));
    }

    n_written += buffer_copy(out, "\r\n"_buf);
    n_written += buffer_copy(out, body);

    return n_written;
}

template <typename Req, buffer_output Out>
std::size_t write_request(Out&& out, Req&& req) {
    return write_request(out, req.start_line(), req.headers(), req.body());
}

}  // namespace neo::http
