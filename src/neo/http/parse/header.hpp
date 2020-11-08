#pragma once

#include <neo/http/parse/common.hpp>

#include <neo/ad_hoc_range.hpp>
#include <neo/const_buffer.hpp>
#include <neo/dynamic_buffer.hpp>
#include <neo/iterator_facade.hpp>

#include <cassert>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace neo::http {

namespace standard_headers {
constexpr std::string_view a_im                             = "A-IM";
constexpr std::string_view accept                           = "Accept";
constexpr std::string_view accept_charset                   = "Accept-Charset";
constexpr std::string_view accept_datetime                  = "Accept-Datetime";
constexpr std::string_view accept_encoding                  = "Accept-Encoding";
constexpr std::string_view accept_language                  = "Accept-Language";
constexpr std::string_view accept_patch                     = "Accept-Patch";
constexpr std::string_view accept_ranges                    = "Accept-Ranges";
constexpr std::string_view access_control_allow_credentials = "Access-Control-Allow-Credentials";
constexpr std::string_view access_control_allow_headers     = "Access-Control-Allow-Headers";
constexpr std::string_view access_control_allow_methods     = "Access-Control-Allow-Methods";
constexpr std::string_view access_control_allow_origin      = "Access-Control-Allow-Origin";
constexpr std::string_view access_control_expose_headers    = "Access-Control-Expose-Headers";
constexpr std::string_view access_control_max_age           = "Access-Control-Max-Age";
constexpr std::string_view access_control_request_headers   = "Access-Control-Request-Headers";
constexpr std::string_view access_control_request_method    = "Access-Control-Request-Method";
constexpr std::string_view age                              = "Age";
constexpr std::string_view allow                            = "Allow";
constexpr std::string_view alt_svc                          = "Alt-Svc";
constexpr std::string_view authorization                    = "Authorization";
constexpr std::string_view cache_control                    = "Cache-Control";
constexpr std::string_view connection                       = "Connection";
constexpr std::string_view content_disposition              = "Content-Disposition";
constexpr std::string_view content_encoding                 = "Content-Encoding";
constexpr std::string_view content_length                   = "Content-Length";
constexpr std::string_view content_location                 = "Content-Location";
constexpr std::string_view content_md5                      = "Content-MD5";
constexpr std::string_view content_range                    = "Content-Range";
constexpr std::string_view content_type                     = "Content-Type";
constexpr std::string_view cookie                           = "Cookie";
constexpr std::string_view date                             = "Date";
constexpr std::string_view delta_base                       = "Delta-Base";
constexpr std::string_view etag                             = "ETag";
constexpr std::string_view expect                           = "Expect";
constexpr std::string_view expires                          = "Expires";
constexpr std::string_view forwarded                        = "Forwarded";
constexpr std::string_view from                             = "From";
constexpr std::string_view host                             = "Host";
constexpr std::string_view http2_settings                   = "HTTP2-Settings";
constexpr std::string_view if_match                         = "If-Match";
constexpr std::string_view if_modified_since                = "If-Modified-Since";
constexpr std::string_view if_none_match                    = "If-None-Match";
constexpr std::string_view if_range                         = "If-Range";
constexpr std::string_view if_unmodified_since              = "If-Unmodified-Since";
constexpr std::string_view im                               = "IM";
constexpr std::string_view last_modified                    = "Last-Modified";
constexpr std::string_view link                             = "Link";
constexpr std::string_view location                         = "Location";
constexpr std::string_view max_forwards                     = "Max-Forwards";
constexpr std::string_view origin                           = "Origin";
constexpr std::string_view p3p                              = "P3P";
constexpr std::string_view pragma                           = "Pragma";
constexpr std::string_view proxy_authenticate               = "Proxy-Authenticate";
constexpr std::string_view proxy_authorization              = "Proxy-Authorization";
constexpr std::string_view public_key_pins                  = "Public-Key-Pins";
constexpr std::string_view range                            = "Range";
constexpr std::string_view referer                          = "Referer";
constexpr std::string_view reply_after                      = "Reply-After";
constexpr std::string_view server                           = "Server";
constexpr std::string_view set_cookie                       = "Set-Cookie";
constexpr std::string_view strict_transport_policy          = "Strict-Transport-Policy";
constexpr std::string_view te                               = "TE";
constexpr std::string_view tk                               = "Tk";
constexpr std::string_view trailer                          = "Trailer";
constexpr std::string_view transfer_encoding                = "Transfer-Encoding";
constexpr std::string_view upgrade                          = "Upgrade";
constexpr std::string_view user_agent                       = "User-Agent";
constexpr std::string_view vary                             = "Vary";
constexpr std::string_view via                              = "Via";
constexpr std::string_view warning                          = "Warning";
constexpr std::string_view www_authenticate                 = "WWW-Authenticate";
constexpr std::string_view x_frame_options                  = "X-Frame-Options";
}  // namespace standard_headers

struct header_bufs {
    /// The buffer that wraps the header field key
    std::string_view key_view;
    /// The buffer that wraps the header field value
    std::string_view value_view;

    /// Trailing data from a parsed header
    neo::const_buffer parse_tail = {};

    static header_bufs parse_without_crlf(neo::const_buffer) noexcept;
    static header_bufs parse(neo::const_buffer cb) noexcept {
        auto h = parse_without_crlf(cb);
        // Check for the CRLF
        if (!h.valid() || !begins_with_crlf(h.parse_tail)) {
            return {};
        }
        h.parse_tail += 2;
        return h;
    }

    bool key_equivalent(std::string_view str) const noexcept;
    bool key_equivalent(neo::const_buffer) const noexcept;

    // The only requirement is that the key be a non-empty string
    constexpr bool valid() const noexcept { return !key_view.empty(); }

    constexpr std::size_t required_write_size() const noexcept {
        return key_view.size() + value_view.size() + 2;  // 2 == len(": ")
    }

    neo::mutable_buffer write(neo::mutable_buffer buf) const noexcept;
};

struct header_iterator : iterator_facade<header_iterator> {
    header_bufs current;

    constexpr header_iterator() = default;

    constexpr header_iterator(const_buffer buf) {
        if (!buf.empty()) {
            current = header_bufs::parse(buf);
        }
    }

    constexpr auto& dereference() const noexcept { return current; }
    void            increment() noexcept {
        assert(!at_end() && "Advance past-the-end of a header_iterator");
        current = header_bufs::parse(current.parse_tail);
    }

    constexpr bool at_end() const noexcept { return !current.valid(); }

    struct sentinel_type {};
};

struct header_lines_buf {
    const_buffer buffer;
    const_buffer parse_tail;

    constexpr bool valid() const noexcept { return buffer.data() != nullptr; }

    constexpr static header_lines_buf parse(const_buffer in) noexcept {
        std::string_view end           = "\r\n\r\n";
        auto             headers_begin = in.data();
        for (; in.size() >= end.size(); in += 1) {
            if (in.first(4).equals_string(end)) {
                auto headers_buf_size = (in.data() - headers_begin) + 2;
                auto headers_buf      = const_buffer(headers_begin, headers_buf_size);
                return {headers_buf, in + end.size()};
            }
        }
        return {};
    }

    constexpr auto iter_headers() const noexcept {
        return ad_hoc_range{header_iterator{buffer}, header_iterator::sentinel_type{}};
    }
};

}  // namespace neo::http
