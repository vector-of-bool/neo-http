#include <neo/http/request.hpp>

#include <neo/string_io.hpp>

#include <catch2/catch.hpp>

#include <map>
#include <string>

struct simple_request {
    neo::http::request_line            req_line;
    std::map<std::string, std::string> headers_;

    auto& start_line() const noexcept { return req_line; }
    auto& headers() const noexcept { return headers_; }

    neo::const_buffer body() const noexcept { return {}; }
};

TEST_CASE("Write a simple request") {
    neo::string_dynbuf_io str;
    simple_request        req;
    req.req_line.http_version     = neo::http::version::v1_1;
    req.req_line.method_view      = "GET";
    req.req_line.target.path_view = "/foo/bar";
    req.headers_["Host"]          = "example.com";
    req.headers_["User-Agent"]    = "Test client";
    neo::http::write_request(str, req);
    CAPTURE(str.read_area_view());
    CHECK(str.read_area_view() ==
        "GET /foo/bar HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: Test client\r\n"
        "\r\n"
        );
}
