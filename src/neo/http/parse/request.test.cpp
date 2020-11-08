#include <neo/http/parse/request.hpp>

#include <catch2/catch.hpp>

using namespace neo;
using namespace neo::http;

using namespace std::string_view_literals;

TEST_CASE("Parse origin-form-targets") {
    struct case_ {
        std::string_view input;
        std::string_view path;
        std::string_view query;
        std::string_view tail;
    };

    auto expect = GENERATE(Catch::Generators::values<case_>({
        {"/foo", "/foo", "", ""},
        {"/foo/bar", "/foo/bar", "", ""},
        {"/foo/bar?query", "/foo/bar", "query", ""},
        {"/", "/", "", ""},
        {"/?", "/", "", ""},
        {"/?q", "/", "q", ""},
        {"/??q", "/", "?q", ""},
        {"/foo tail", "/foo", "", " tail"},
        {"/foo?bar=baz;qux=meow tail", "/foo", "bar=baz;qux=meow", " tail"},
    }));
    auto actual = origin_form_target::parse_next(const_buffer(expect.input));
    CHECK(actual.path_view == expect.path);
    CHECK(actual.query_view == expect.query);
    CHECK(std::string_view(actual.parse_tail) == expect.tail);
}

TEST_CASE("Fail invalid origin targets") {
    std::string_view invalids[] = {
        "/%",
        "/%%%",
        "%",
        "",
        "/%f0",
        "fail",
        "fail/bad",
        "%fail/bad",
        "%f",
        "%8g",
    };
    for (auto path : invalids) {
        auto res = origin_form_target::parse_next(const_buffer(path));
        CHECK_FALSE(res.valid());
    }
}

TEST_CASE("Parse request lines") {
    struct case_ {
        std::string_view input;
        std::string_view method;
        std::string_view target;
        std::string_view query;
        http::version    version;
    };
    auto expect = GENERATE(Catch::Generators::values<case_>({
        {"GET /foo HTTP/1.1\r\n", "GET", "/foo", "", version::v1_1},
        {"POST /foo HTTP/1.1\r\n", "POST", "/foo", "", version::v1_1},
        {"POST /foo?cat HTTP/1.1\r\n", "POST", "/foo", "cat", version::v1_1},
    }));
    auto actual = request_line::parse(const_buffer(expect.input));
    CHECK(actual.valid());
    CHECK(actual.method_view == expect.method);
    CHECK(actual.target.path_view == expect.target);
    CHECK(actual.target.query_view == expect.query);
    CHECK(actual.http_version == expect.version);
    CHECK(actual.parse_tail.empty());
}

TEST_CASE("Write origin targets") {
    std::string str;

    origin_form_target t;
    t.path_view  = "/foo";
    t.has_query  = true;
    t.query_view = "bar";
    CHECK(t.byte_size() == 8);

    str.resize(t.byte_size());
    t.write(mutable_buffer(str));
    CHECK(str == "/foo?bar");

    t.has_query = false;
    str.resize(t.byte_size());
    t.write(mutable_buffer(str));
    CHECK(str == "/foo");

    t.has_query  = true;
    t.query_view = "";
    str.resize(t.byte_size());
    t.write(mutable_buffer(str));
    CHECK(str == "/foo?");
}

TEST_CASE("Write a request line") {
    std::string  str;
    request_line req;
    req.method_view      = "GET";
    req.target.path_view = "/foo";
    req.http_version     = version::v1_1;

    CHECK(req.byte_size() == 19);
    str.resize(req.byte_size());
    req.write(mutable_buffer(str));
    CHECK(str == "GET /foo HTTP/1.1\r\n");
}

TEST_CASE("Read a request head") {
    std::string_view head
        = "GET /foo/bar HTTP/1.1\r\n"
          "Content-Length: 20\r\n"
          "Host: example.com\r\n"
          "\r\n";
    auto req_head = neo::http::request_head::parse(neo::const_buffer(head));
    CHECK(req_head.valid());
    // The headers_buf will always contain one of the two trailing newlines
    CHECK(std::string_view(req_head.headers.buffer)
          == "Content-Length: 20\r\nHost: example.com\r\n");
    // The second of the two trailing CRLF will not be considered in parse_tail
    CHECK(req_head.parse_tail.empty());
    CHECK(req_head.start_line.target.path_view == "/foo/bar");
    CHECK(req_head.start_line.method_view == "GET");
    CHECK(req_head.start_line.http_version == neo::http::version::v1_1);

    auto iter = req_head.headers.iter_headers().begin();
    REQUIRE_FALSE(iter.at_end());
    CHECK(iter->key_view == "Content-Length");
    CHECK(iter->value_view == "20");

    ++iter;
    REQUIRE_FALSE(iter.at_end());
    CHECK(iter->key_view == "Host");
    CHECK(iter->value_view == "example.com");

    ++iter;
    CHECK(iter.at_end());

    for (auto header : req_head.headers.iter_headers()) {
        CHECK(header.valid());
    }
}

TEST_CASE("Reject bad request heads") {
    std::string_view bads[] = {
        // Missing HTTP version
        "GET /foo/bar\r\n"
        "\r\n",
        // Missing trailing CRLF
        "GET /foo/bar HTTP/1.1\r\n"
        "Foo: Bar\r\n",
        // Invalid HTTP version
        "GET /foo/bar HTTP/5.2\r\n"
        "Foo: Bar\r\n"
        "\r\n",
    };
    for (auto bad : bads) {
        CAPTURE(bad);
        auto req_head = neo::http::request_head::parse(neo::const_buffer(bad));
        CHECK_FALSE(req_head.valid());
    }
}

TEST_CASE("Iterate with bad headers") {
    std::string_view bad
        = "GET /foo/bar HTTP/1.1\r\n"
          "Content-Length: 3\r\n"
          "Bad line\r\n"
          "Okay-Line: Foo\r\n"
          "\r\n";
    auto req_head = neo::http::request_head::parse(neo::const_buffer(bad));

    // The request-head parsing won't validate header lines: It will only look for the first
    // CRLFCRLF sequence
    REQUIRE(req_head.valid());

    auto iter = req_head.headers.iter_headers().begin();
    REQUIRE_FALSE(iter.at_end());
    CHECK(iter->key_equivalent("content-length"));
    CHECK(iter->value_view == "3");

    ++iter;
    CHECK(iter.at_end());  // We've got a bad header, so the iterator is done
    CHECK_FALSE(iter->valid());
}
