#pragma once

#include "./message.hpp"
#include <neo/http/parse/common.hpp>
#include <neo/http/parse/header.hpp>
#include <neo/http/parse/version.hpp>
#include <neo/http/version.hpp>

#include <neo/ad_hoc_range.hpp>
#include <neo/buffer_algorithm/copy.hpp>
#include <neo/buffer_sink.hpp>
#include <neo/const_buffer.hpp>

#include <string_view>

namespace neo::http {

struct origin_form_target {
    std::string_view path_view;
    std::string_view query_view;
    bool             has_query = false;

    const_buffer parse_tail;

    static origin_form_target parse_next(const_buffer) noexcept;

    constexpr bool valid() const noexcept { return !path_view.empty(); }

    template <buffer_output Out>
    std::size_t write(Out&& out_) const noexcept {
        auto&& out      = ensure_buffer_sink(out_);
        auto   n_copied = buffer_copy(out, as_buffer(path_view));
        if (has_query) {
            auto bufs = {const_buffer("?"), const_buffer(query_view)};
            n_copied += buffer_copy(out, bufs);
        }
        return n_copied;
    }

    constexpr std::size_t byte_size() const noexcept {
        return path_view.size()                   //
            + (has_query ? query_view.size() + 1  // +1 If we have a query, we need to write a `?`
                         : 0);
    }
};

struct request_line {
    std::string_view   method_view;
    origin_form_target target;
    version            http_version;

    const_buffer parse_tail;

    static request_line parse(const_buffer buf) noexcept;

    constexpr bool valid() const noexcept {
        return !method_view.empty() && http_version != version::invalid;
    }

    template <buffer_output Out>
    std::size_t write(Out&& out_) const noexcept {
        auto&& out      = ensure_buffer_sink(out_);
        auto   bufs_1   = {const_buffer(method_view), const_buffer(" ")};
        auto   n_copied = buffer_copy(out, bufs_1);
        n_copied += target.write(out);
        auto bufs2 = {const_buffer(" "), version_buf(http_version), const_buffer("\r\n")};
        return buffer_copy(out, bufs2);
    }

    constexpr std::size_t byte_size() const noexcept {
        return method_view.size() + 1  // +1 for SP
            + target.byte_size() + 1   // +1 for SP
            + version_buf_size + 2;    // +2 for CRLF
    }
};

struct request_head : message_head<request_head, request_line> {};

}  // namespace neo::http
