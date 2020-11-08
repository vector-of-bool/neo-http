#include <neo/http/parse/token.hpp>

#include <catch2/catch.hpp>

using namespace neo;
using namespace neo::http;
using namespace std::string_view_literals;

TEST_CASE("Parse simple tokens") {
    struct case_ {
        std::string_view str;
        std::string_view token;
    };
    auto expect = GENERATE(Catch::Generators::values<case_>({
        {"First second", "First"},
        {"GET /some/url HTTP/1.1", "GET"},
    }));
    auto actual = token::parse_next(const_buffer(expect.str));
    CHECK(actual.view == expect.token);
    CHECK(std::string_view(actual.parse_tail) == expect.str.substr(expect.token.size()));
}
