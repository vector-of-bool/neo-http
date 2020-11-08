#include <neo/as_buffer.hpp>
#include <neo/http/parse/status.hpp>

#include <catch2/catch.hpp>

using namespace std::string_view_literals;
using namespace neo::literals;

TEST_CASE("Parse a status") {
    auto line = neo::http::status_line::parse("HTTP/1.1 200 Okay\r\n"_buf);
    CHECK(line.status == 200);
    CHECK(line.http_version == neo::http::version::v1_1);
}

TEST_CASE("Cases") {
    struct case_ {
        std::string_view line;
        int              status;
        std::string_view phrase;
    };

    auto expect = GENERATE(Catch::Generators::values<case_>({
        {"HTTP/1.1 200 Okay\r\n", 200, "Okay"},
        {"HTTP/1.1 201 asdf\r\n", 201, "asdf"},
        {"HTTP/1.0 201 This is a random status line with whitespace in it\r\n",
         201,
         "This is a random status line with whitespace in it"},
    }));
    INFO("Test parse line: " << expect.line);
    auto line = neo::http::status_line::parse(neo::const_buffer(expect.line));
    CHECK(line.status == expect.status);
    CHECK(line.phrase_view == expect.phrase);
}

TEST_CASE("Bad statuses") {
    std::string_view bad_lines[] = {
        "HTTP/1.  200 Okay\r\n",    // Bad version
        "HTTP/1.2 200 Okay\r\n",    // Bad HTTP version
        "HTTP/1.1 200 Okay\n\r\n",  // Extra \n
        "HTTP/1.1 200 Okay\r\r\n",  // Extra \r
        "HTTP/1.1 000 Okay\r\n",    // Invalid code
        "HTTP/1.1 20 Okay\r\n",     // Invalid code
        "HTTP/1.1 2000 Okay\r\n",   // Invalid code
        "HTTP/1.1 2 0 Okay\r\n",    // Invalid code
        "HTTP/1.1 -20 Okay\r\n",    // Invalid code
        "HTTP/1.1 0x1 Okay\r\n",    // Invalid code
        "HTTP/1.1 000 Okay\r\n",    // Invalid code
        "HTTP/1.1 0b0 Okay\r\n",    // Invalid code
        "HTTP/1.1 0c0 Okay\r\n",    // Invalid code
        "HTTP/1.1 200\r\n",         // Missing message
    };
    for (auto line : bad_lines) {
        INFO("Checking bad status line:");
        INFO(line);
        auto res = neo::http::status_line::parse(neo::const_buffer(line));
        CHECK(res.http_version == neo::http::version::invalid);
    }
}

TEST_CASE("Write statuses") {
    std::string            str;
    neo::http::status_line st;
    st.http_version = neo::http::version::v1_1;
    st.phrase_view  = "Okey Dokey";
    st.status       = 200;
    CHECK(st.required_write_size() == (8 + 1 + 10 + 1 + 3 + 2));
    str.resize(st.required_write_size());
    auto retbuf = st.write(neo::mutable_buffer(str));
    CHECK(retbuf.empty());
    CHECK(str == "HTTP/1.1 200 Okey Dokey\r\n");
}
