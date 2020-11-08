#include "./token.hpp"

#include <algorithm>

using namespace std::string_view_literals;

neo::http::token neo::http::token::parse_next(neo::const_buffer buf) noexcept {
    auto is_separator_char = [&](auto c) {
        return "()<>@,;:\\\"/[]?={} \t"sv.find(static_cast<char>(c)) != std::string_view::npos;
    };
    auto is_token_char = [&](auto c_) {
        auto c = static_cast<char>(c_);
        return (  // The range of VCHAR:
            (c >= '\x21' && c <= '\x7e')
            // Excluding "separator" characters
            && !is_separator_char(c));
    };

    auto token_stop  = std::find_if_not(buf.data(), buf.data_end(), is_token_char);
    auto token_size  = token_stop - buf.data();
    auto [tok, tail] = buf.split(token_size);
    return {std::string_view(tok), tail};
}
