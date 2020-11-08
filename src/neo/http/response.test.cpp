#include <neo/http/response.hpp>

#include <neo/pathological_buffer_range.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Read a basic HTTP response") {
    auto res_str = neo::const_buffer(
        "HTTP/1.1 200 Okay\r\n"
        "Content-Length: 12\r\n"
        "Content-Disposition: lol test\r\n"
        "\r\n"
        "Message body");
    neo::http::simple_response resp;

    SECTION("Single contiguous buffer") {
        resp = neo::http::read_response_head<neo::http::simple_response>(res_str);
    }
    SECTION("Pathological buffer") {
        resp = neo::http::read_response_head<neo::http::simple_response>(
            neo::pathological_buffer_range(res_str));
    }

    CHECK(resp.status == 200);
    CHECK(resp.status_message == "Okay");
    CHECK(resp.version == neo::http::version::v1_1);
    CHECK(resp.headers["Content-Length"].value == "12");
    CHECK(resp.headers["Content-Disposition"].value == "lol test");
    CHECK(resp.head_byte_size == 72);
    auto body_buf = res_str + resp.head_byte_size;
    CHECK(std::string_view(body_buf) == "Message body");
}

TEST_CASE("Read an HTTP response with a body") {
    auto res_str = neo::const_buffer(
        "HTTP/1.1 200 Okay\r\n"
        "Content-Length: 12\r\n"
        "Content-Disposition: lol test\r\n"
        "\r\n"
        "Message body");

    neo::string_dynbuf_io body_io;
    neo::http::read_response(body_io, res_str);
    CHECK(body_io.read_area_view() == "Message body");
}

TEST_CASE("Read an encoded HTTP response with a body") {
    auto res_str = neo::const_buffer(
        "HTTP/1.1 200 Okay\r\n"
        "Transfer-Encoding: null-terminated\r\n"
        "Content-Disposition: lol test\r\n"
        "\r\n"
        "Message body\n"
        "I am on another line\n\x00"
        "other random data");

    neo::string_dynbuf_io body_io;

    auto nread = neo::http::read_response(body_io, res_str, [](auto te, auto&& in) {
        // This is an extremely simple transfer-encoding that reads bytes until
        // it finds the null terminator.
        CHECK(te == "null-terminated");
        auto tr = [](auto out, auto in) {
            auto null_pos = std::string_view(in).find_first_of('\x00');
            auto n_copied = buffer_copy(out, in, null_pos);
            return neo::simple_transform_result{
                .bytes_written = n_copied,
                .bytes_read    = n_copied,
                .done          = n_copied == null_pos,
            };
        };
        return neo::buffer_transform_source{in, tr};
    });
    CHECK(nread == 34);
    CHECK(body_io.read_area_view() == "Message body\nI am on another line\n");
}

TEST_CASE("Read an chunked HTTP response with a body") {
    auto res_str = neo::const_buffer(
        "HTTP/1.1 200 Okay\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Disposition: lol test\r\n"
        "\r\n"
        "d\r\n"
        "Message body\n\r\n"
        "15\r\n"
        "I am on another line\n\r\n"
        "0\r\n\r\n"
        "[Not in the message]");

    neo::string_dynbuf_io body_io;

    auto nread = neo::http::read_response(body_io, res_str);
    CHECK(nread == 34);
    CHECK(body_io.read_area_view() == "Message body\nI am on another line\n");

    auto patho = neo::pathological_buffer_range{res_str};
    body_io.clear();
    nread = neo::http::read_response(body_io, patho);
    CHECK(nread == 34);
    CHECK(body_io.read_area_view() == "Message body\nI am on another line\n");
}
