#include <neo/http/parse/version.hpp>

#include <catch2/catch.hpp>

using namespace std::string_view_literals;

using namespace neo;
using namespace neo::http;

TEST_CASE("Parse HTTP versions") {
    CHECK(parse_version(const_buffer("HTTP/1.1"sv)) == version::v1_1);
    CHECK(parse_version(const_buffer("HTTP/1.0"sv)) == version::v1_0);

    // Invalid version strings:
    CHECK(parse_version(const_buffer("HTTP/1.2"sv)) == version::invalid);
    CHECK(parse_version(const_buffer("HTTP/1."sv)) == version::invalid);
    CHECK(parse_version(const_buffer("HTTP/1.11"sv)) == version::invalid);
    CHECK(parse_version(const_buffer("nope"sv)) == version::invalid);
    CHECK(parse_version(const_buffer(""sv)) == version::invalid);
}

TEST_CASE("Get HTTP version string buffers") {
    CHECK(version_buf(version::v1_1).equals_string("HTTP/1.1"sv));
    CHECK(version_buf(version::v1_0).equals_string("HTTP/1.0"sv));
}

TEST_CASE("Write version buffers") {
    std::string out;
    out.resize(version_buf_size);

    auto tail = write_version(mutable_buffer(out), version::v1_1);
    // Should have written the entire buffer:
    CHECK(tail.empty());
    CHECK(out == "HTTP/1.1");

    tail = write_version(mutable_buffer(out), version::v1_0);
    CHECK(tail.empty());
    CHECK(out == "HTTP/1.0");
}
