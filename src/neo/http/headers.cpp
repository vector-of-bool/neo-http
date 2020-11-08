#include "./headers.hpp"

#include <locale>

bool neo::http::header_key_equivalent(std::string_view a, std::string_view b) noexcept {
    static auto& c_locale = std::use_facet<std::ctype<char>>(std::locale("C"));
    return std::equal(a.cbegin(), a.cend(), b.cbegin(), b.cend(), [&](char a, char b) {
        return c_locale.tolower(a) == c_locale.tolower(b);
    });
}