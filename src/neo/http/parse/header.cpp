#include "./header.hpp"

#include "./abnf.hpp"
#include "./token.hpp"
#include <neo/http/headers.hpp>

#include <neo/buffer_algorithm.hpp>
#include <neo/concepts.hpp>
#include <neo/iterator_concepts.hpp>
#include <neo/test_concept.hpp>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <locale>
#include <string_view>

using namespace std::string_view_literals;

NEO_TEST_CONCEPT(neo::input_iterator<neo::http::header_iterator>);

neo::http::header_bufs neo::http::header_bufs::parse_without_crlf(neo::const_buffer line) noexcept {
    // The field name is just a token.
    auto name_tok = token::parse_next(line);
    line          = name_tok.parse_tail;

    // We must be followed immediately by a colon
    if (!name_tok.valid() || line.empty() || line[0] != std::byte{':'}) {
        return {};
    }
    // Skip the colon
    line += 1;

    // Skip leading whitespace on the value
    while (!line.empty() && parse_detail::WSP.contains(static_cast<char>(line[0]))) {
        line += 1;
    }

    // Create a buffer from the value, removing trailing whitespace
    auto             content_begin_buf = line;
    const std::byte* content_end       = line.data();
    while (!line.empty() && parse_detail::VCHAR.contains(static_cast<char>(line[0]))) {
        // Consume a field-content part
        line += 1;
        content_end = line.data();
        // Skip all whitespace
        while (!line.empty() && parse_detail::WSP.contains(static_cast<char>(line[0]))) {
            line += 1;
        }
    }

    auto field_buf = content_begin_buf.first(content_end - content_begin_buf.data());

    return {std::string_view(name_tok.view), std::string_view(field_buf), line};
}

bool neo::http::header_bufs::key_equivalent(neo::const_buffer buf) const noexcept {
    return key_equivalent(std::string_view(reinterpret_cast<const char*>(buf.data()), buf.size()));
}

bool neo::http::header_bufs::key_equivalent(std::string_view other_key) const noexcept {
    return header_key_equivalent(key_view, other_key);
}

neo::mutable_buffer neo::http::header_bufs::write(neo::mutable_buffer out) const noexcept {
    auto bufs = {
        const_buffer(key_view),
        const_buffer(": "),
        const_buffer(value_view),
    };
    assert(out.size() >= buffer_size(bufs));
    return out + buffer_copy(out, bufs);
}