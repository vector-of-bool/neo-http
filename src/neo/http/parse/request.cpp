#include "./request.hpp"

#include <iostream>

#include <neo/buffer_algorithm.hpp>

#include "./abnf.hpp"
#include "./token.hpp"
#include "./version.hpp"

using namespace std::string_view_literals;

neo::http::origin_form_target
neo::http::origin_form_target::parse_next(neo::const_buffer buf) noexcept {
    constexpr static origin_form_target invalid_ret = {{}, {}, false, {}};
    if (buf.empty() || buf[0] != std::byte{'/'}) {
        return invalid_ret;
    }
    using namespace parse_detail;

    const auto full_buf = buf;

    enum pchar_res {
        pchar_ok,
        pchar_invalid,
        pchar_done,
    };

    auto read_pchar = [&] {
        if (PCHAR_BASIC_CHARS.contains(char(buf[0]))) {
            buf += 1;
            return pchar_ok;
        }
        if (buf[0] == std::byte{'%'}) {
            if (buf.size() < 3) {
                return pchar_invalid;
            }
            if (HEXDIG.contains(char(buf[1])) && HEXDIG.contains(char(buf[2]))) {
                buf += 3;
                return pchar_ok;
            }
            return pchar_invalid;
        }
        return pchar_done;
    };

    while (!buf.empty() && buf[0] == std::byte{'/'}) {
        buf += 1;
        while (!buf.empty()) {
            auto pcr = read_pchar();
            if (pcr == pchar_invalid) {
                return invalid_ret;
            } else if (pcr == pchar_done) {
                break;
            } else {
                // More path segment to parse
            }
        }
    }

    auto path_len = buf.data() - full_buf.data();
    auto path_buf = full_buf.first(path_len);

    auto query_full_buf = buf;
    bool have_query     = !buf.empty() && buf[0] == std::byte{'?'};
    query_full_buf += have_query ? 1 : 0;

    while (!buf.empty()) {
        if (buf[0] == std::byte{'/'} || buf[0] == std::byte{'?'}) {
            buf += 1;
            continue;
        }
        auto pcr = read_pchar();
        if (pcr == pchar_invalid) {
            return invalid_ret;
        } else if (pcr == pchar_done) {
            break;
        } else {
            // More query text to parse
        }
    }

    auto query_len = buf.data() - query_full_buf.data();
    auto query_buf = query_full_buf.first(query_len);

    return {std::string_view(path_buf), std::string_view(query_buf), have_query, buf};
}

neo::http::request_line neo::http::request_line::parse(neo::const_buffer buf) noexcept {
    constexpr static request_line invalid_ret = {"", {}, version::invalid, {}};

    auto method_buf = token::parse_next(buf);
    if (!method_buf.valid()) {
        return invalid_ret;
    }

    buf = method_buf.parse_tail;
    if (buf.size() < 2 || buf[0] != std::byte{' '}) {
        return invalid_ret;
    }

    buf += 1;

    /// TODO: Several different request-targets are supported. We need to support
    /// them all.
    auto target = origin_form_target::parse_next(buf);
    if (!target.valid()) {
        return invalid_ret;
    }

    buf = target.parse_tail;
    if (buf.empty() || buf[0] != std::byte{' '}) {
        return invalid_ret;
    }

    buf += 1;

    if (buf.size() < version_buf_size) {
        return invalid_ret;
    }

    auto ver_buf = buf.first(version_buf_size);
    auto ver     = parse_version(ver_buf);
    if (ver == version::invalid) {
        return invalid_ret;
    }

    buf += ver_buf.size();

    if (!begins_with_crlf(buf)) {
        return invalid_ret;
    }
    buf += 2;

    return {std::string_view(method_buf.view), target, ver, buf};
}

// neo::mutable_buffer neo::http::origin_form_target::write(neo::mutable_buffer out) const noexcept
// {
//     assert(out.size() >= required_write_size());

//     assert(out.size() >= path_view.size());
//     out += buffer_copy(out, const_buffer(path_view));

//     if (has_query) {
//         auto bufs = {const_buffer("?"), const_buffer(query_view)};
//         assert(out.size() >= buffer_size(bufs));
//         out += buffer_copy(out, bufs);
//     }
//     return out;
// }

// neo::mutable_buffer neo::http::request_line::write(neo::mutable_buffer out) const noexcept {
//     assert(out.size() >= required_write_size());

//     auto head = {const_buffer(method_view), const_buffer(" ")};
//     assert(out.size() >= buffer_size(head));
//     out += buffer_copy(out, head);

//     out = target.write(out);

//     auto tail = {
//         const_buffer(" "),
//         version_buf(http_version),
//         const_buffer("\r\n"),
//     };
//     return out + buffer_copy(out, tail);
// }

// neo::http::request_head neo::http::request_head::parse(neo::const_buffer buf) noexcept {
//     auto start = request_line::parse(buf);
//     if (!start.valid()) {
//         return {};
//     }
//     auto headers_buf_full = buf = start.parse_tail;

//     // Find the next `\r\n\r\n` sequence that ends the request head
//     std::string_view end = "\r\n\r\n";
//     for (; buf.size() >= end.size(); buf += 1) {
//         if (buf.first(4).equals_string(end)) {
//             auto headers_buf_size = (buf.data() - headers_buf_full.data()) + 2;
//             auto headers_buf      = headers_buf_full.first(headers_buf_size);
//             return {start, headers_buf, buf + end.size()};
//         }
//     }
//     // No tail was found. No good.
//     return {};
// }
