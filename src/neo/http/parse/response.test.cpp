#include <neo/http/parse/response.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Read a response head") {
    std::string_view head
        = "HTTP/1.1 200 Okey Dokey\r\n"
          "Content-Length: 20\r\n"
          "Content-Type: application/json\r\n"
          "\r\n";
    auto req_head = neo::http::response_head::parse(neo::const_buffer(head));
    CHECK(req_head.valid());
    // The headers_buf will always contain one of the two trailing newlines
    CHECK(std::string_view(req_head.headers.buffer)
          == "Content-Length: 20\r\nContent-Type: application/json\r\n");
    // The second of the two trailing CRLF will not be considered in parse_tail
    CHECK(req_head.parse_tail.empty());
    CHECK(req_head.start_line.status == 200);
    CHECK(req_head.start_line.phrase_view == "Okey Dokey");
    CHECK(req_head.start_line.http_version == neo::http::version::v1_1);

    auto iter = req_head.headers.iter_headers().begin();
    REQUIRE_FALSE(iter.at_end());
    CHECK(iter->key_view == "Content-Length");
    CHECK(iter->value_view == "20");

    ++iter;
    REQUIRE_FALSE(iter.at_end());
    CHECK(iter->key_view == "Content-Type");
    CHECK(iter->value_view == "application/json");

    ++iter;
    CHECK(iter.at_end());

    for (auto header : req_head.headers.iter_headers()) {
        CHECK(header.valid());
    }
}

TEST_CASE("Reject bad response heads") {
    std::string_view bads[] = {
        // Missing HTTP version
        "200 Okay\r\n"
        "\r\n",
        // Missing trailing CRLF
        "HTTP/1.1 200 Okay\r\n"
        "Foo: Bar\r\n",
        // Invalid HTTP version
        "HTTP/5.2 200 Okay\r\n"
        "Foo: Bar\r\n"
        "\r\n",
    };
    for (auto bad : bads) {
        CAPTURE(bad);
        auto req_head = neo::http::response_head::parse(neo::const_buffer(bad));
        CHECK_FALSE(req_head.valid());
    }
}
