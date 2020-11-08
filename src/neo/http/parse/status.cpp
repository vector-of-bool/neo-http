#include "./status.hpp"
#include "./abnf.hpp"
#include "./version.hpp"

#include <neo/buffer_algorithm.hpp>

#include <charconv>
#include <cstring>
#include <string_view>

using namespace std::string_view_literals;

neo::http::status_line neo::http::status_line::parse(neo::const_buffer cbuf) noexcept {
    static constexpr status_line invalid_ret = {version::invalid, -1, "", {}};

    // All HTTP statuses have a min length. Check it now so we don't have to check repeatedly
    static constexpr std::string_view min_len_str = "HTTP/1.N NNN \r\n";
    if (cbuf.size() < min_len_str.size()) {
        return invalid_ret;
    }

    // Parse a version start the same
    constexpr auto ver_buf_len = std::string_view("HTTP/1.x").length();
    auto           ver_buf     = cbuf.first(ver_buf_len);
    auto           ver         = parse_version(ver_buf);
    if (ver == version::invalid) {
        return invalid_ret;
    }
    cbuf += ver_buf.size();

    // We now expect one space character
    if (cbuf.empty() || cbuf[0] != std::byte{' '}) {
        return invalid_ret;
    }
    cbuf += 1;

    // Now a status code, always three digits followed by a space
    if (cbuf.size() < 4) {
        return invalid_ret;
    }
    const auto num_begin   = reinterpret_cast<const char*>(cbuf.data());
    const auto num_end     = num_begin + 3;
    int        status_code = -42;
    auto       conv_res    = std::from_chars(num_begin, num_end, status_code);
    if (
        // Parse error:
        conv_res.ec != std::errc()
        // Didn't parse everything:
        || conv_res.ptr != num_end
        // Bad status code (must be a positive non-zero integer)
        || status_code <= 0) {
        return invalid_ret;
    }
    // num_end should point at a space
    if (*num_end != ' ') {
        return invalid_ret;
    }

    // Skip the three digits + one space
    cbuf += 4;

    // We should now have at least one byte for the reason phrase
    if (cbuf.empty()) {
        return invalid_ret;
    }

    auto reason_chars = parse_detail::WSP | parse_detail::VCHAR;
    auto phrase_begin = cbuf.data();
    // Check each reason phrase char:
    while (!cbuf.empty() && reason_chars.contains(char(cbuf[0]))) {
        cbuf += 1;
    }

    auto phrase = std::string_view(const_buffer(phrase_begin, cbuf.data() - phrase_begin));

    // There will be a CRLF, followed by any trailing bytes
    if (!begins_with_crlf(cbuf)) {
        return invalid_ret;
    }
    // Skip the CRLF. This is the parse tail
    cbuf += 2;

    // We parsed it good!
    return {ver, status_code, phrase, cbuf};
}

std::string_view neo::http::default_phrase(int code) noexcept {
    switch (code) {
#define PHRASE(Num, Phrase)                                                                        \
    case Num:                                                                                      \
        return Phrase
        // 1xx
        PHRASE(100, "Continue");
        PHRASE(101, "Switching Protocols");
        PHRASE(103, "Early Hints");
        // 2xx
        PHRASE(200, "Okay");
        PHRASE(201, "Created");
        PHRASE(202, "Accepted");
        PHRASE(203, "Non-Authoritative Information");
        PHRASE(204, "No Content");
        PHRASE(205, "Reset Content");
        PHRASE(206, "Partial Content");
        PHRASE(207, "Multi-Status");
        PHRASE(208, "IM Used");
        // 300
        PHRASE(300, "Multiple Choices");
        PHRASE(301, "Moved Permanently");
        PHRASE(302, "Found");
        PHRASE(303, "See Other");
        PHRASE(304, "Not Modified");
        PHRASE(305, "Use Proxy");
        PHRASE(306, "Switch Proxy");
        PHRASE(307, "Temporary Redirect");
        PHRASE(308, "Permanent Redirect");
        // 4xx
        PHRASE(400, "Bad Request");
        PHRASE(401, "Unauthorized");
        PHRASE(402, "Payment Required");
        PHRASE(403, "Forbidden");
        PHRASE(404, "Not Found");
        PHRASE(405, "Method Not Allowed");
        PHRASE(406, "Not Acceptable");
        PHRASE(407, "Proxy Authentication Required");
        PHRASE(408, "Request Timeout");
        PHRASE(409, "Conflict");
        PHRASE(410, "Gone");
        PHRASE(411, "Length Required");
        PHRASE(412, "Precondition Failed");
        PHRASE(413, "Payload Too Large");
        PHRASE(414, "URI Too Long");
        PHRASE(415, "Unsupported Media Type");
        PHRASE(416, "Range Not Satisfiable");
        PHRASE(417, "Expectation Failed");
        PHRASE(418, "I'm a teapot");
        PHRASE(421, "Misdirected Requested");
        PHRASE(422, "Unprocessable Entity");
        PHRASE(423, "Locked");
        PHRASE(424, "Failed Dependency");
        PHRASE(425, "Too Early");
        PHRASE(426, "Upgrade Required");
        PHRASE(428, "Precondition Required");
        PHRASE(429, "Too Many Requests");
        PHRASE(431, "Request Header Fields Too Large");
        PHRASE(451, "Unavailable For Legal Reasons");
        // 5xx
        PHRASE(500, "Internal Server Error");
        PHRASE(501, "Not Implemented");
        PHRASE(502, "Bad Gateway");
        PHRASE(503, "Service Unavailable");
        PHRASE(504, "Gateway Timeout");
        PHRASE(505, "HTTP Version Not Supported");
        PHRASE(506, "Variant Also Negotiates");
        PHRASE(507, "Insufficient Storage");
        PHRASE(508, "Loop Detected");
        PHRASE(510, "Not Extended");
        PHRASE(511, "Network Authentication Required");
    default:
        return "";
#undef PHRASE
    }
}

neo::mutable_buffer neo::http::status_line::write(neo::mutable_buffer out) const noexcept {
    neo_assert(expects, valid(), "Cannot write() an invalid status line");
    neo_assert(expects,
               out.size() >= required_write_size(),
               "Output buffer is not large enough for status line",
               out.size(),
               required_write_size());

    char status_chars[3];
    std::to_chars(status_chars, status_chars + 3, status);

    auto bufs = {
        version_buf(http_version),
        const_buffer(" "),
        const_buffer(byte_pointer(status_chars), 3),
        const_buffer(" "),
        const_buffer(phrase_view),
        const_buffer("\r\n"),
    };
    neo_assert(invariant,
               buffer_size(bufs) == required_write_size(),
               "Generated buffer size is incorrect",
               required_write_size(),
               buffer_size(bufs));
    return out + buffer_copy(out, bufs);
}
