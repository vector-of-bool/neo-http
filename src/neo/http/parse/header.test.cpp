#include <neo/http/parse/header.hpp>

#include <catch2/catch.hpp>

#include <neo/bytes.hpp>
#include <neo/dynamic_buffer.hpp>

TEST_CASE("Parse a simple header line") {
    neo::http::header_bufs header{"Content-Length", "311"};
    CHECK(header.key_equivalent("Content-Length"));
    CHECK(header.key_equivalent("content-length"));
    CHECK(header.key_equivalent("CONTENT-LENGTH"));
    CHECK_FALSE(header.key_equivalent("Bogus"));
}

TEST_CASE("Cases") {
    struct case_ {
        std::string_view line;
        std::string_view key;
        std::string_view value;
        std::string_view tail = "";
    };

    using namespace neo::http;

    auto expect = GENERATE(Catch::Generators::values<case_>({
        // We are case-insensitive in our comparisons
        {"Content-Length: 22", "content-length", "22"},
        {"Content-Length: 22", "CONTENT-LENGTH", "22"},
        {"Content-Length:    22", "CONTENT-LENGTH", "22"},
        {"Content-LENGTH:    22", "CONTENT-LENGTH", "22"},
        {"Content-LENGTH:    22   ", "CONTENT-LENGTH", "22"},
        {"Content-LENGTH:22", "CONTENT-LENGTH", "22"},

        {"Foo: Bar", "Foo", "Bar"},
        {"Foo:", "Foo", ""},
        {"Foo: Bar; Baz", "Foo", "Bar; Baz"},
        {"Foo: Bar; Baz   ", "Foo", "Bar; Baz"},
        {"Foo: Bar\r\n", "Foo", "Bar", "\r\n"},
        {"Foo: Bar\r\nTrailing", "Foo", "Bar", "\r\nTrailing"},

        // Bad cases:
        {": No-key", "", ""},
        {"Foo : Extra-Space", "", ""},
        {" Foo : Extra-Space", "", ""},
    }));
    CAPTURE(expect.line);
    auto actual = neo::http::header_bufs::parse_without_crlf(neo::const_buffer(expect.line));
    CAPTURE(expect.key);
    CAPTURE(actual.key_view);
    CAPTURE(expect.value);
    CAPTURE(actual.value_view);
    CHECK(actual.key_equivalent(expect.key));
    CHECK(actual.value_view == expect.value);
    CHECK(actual.parse_tail.equals_string(expect.tail));
}

TEST_CASE("Write headers") {
    neo::bytes             buf;
    neo::http::header_bufs header;
    header.key_view   = neo::http::standard_headers::content_length;
    header.value_view = "1729";
    buf.resize(header.required_write_size());
    auto remaining = header.write(neo::mutable_buffer(buf));
    CHECK(remaining.empty());

    neo::bytes       expect;
    std::string_view expect_str = "Content-Length: 1729";
    expect.resize(expect_str.size(), neo::bytes::uninit);
    neo::buffer_copy(neo::mutable_buffer(expect), neo::const_buffer(expect_str));

    CHECK(buf == expect);
}
