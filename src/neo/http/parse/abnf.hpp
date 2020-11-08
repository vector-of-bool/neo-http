#pragma once

// Refer: RFC7230, rfc5234, RFC3986

/**
 * This defines a set of character classes that are used throughout the parser
 */
namespace neo::http::parse_detail {

template <typename Left, typename Right>
struct either {
    constexpr static bool contains(char c) noexcept {
        return Left::contains(c) || Right::contains(c);
    }

    template <typename Other>
    either<either, Other> operator|(Other) const noexcept {
        return {};
    }
};

template <char... Cs>
struct char_set_t {
    constexpr static bool contains(char c) noexcept { return ((Cs == c) || ...); }

    template <typename Other>
    either<char_set_t, Other> operator|(Other) const noexcept {
        return {};
    }
};

template <char... Cs>
inline char_set_t<Cs...> char_set = {};

template <char Min, char Max>
struct char_range_t {
    static_assert(Min < Max);
    constexpr static bool contains(char c) noexcept { return c >= Min && c <= Max; }

    template <typename Other>
    either<char_range_t, Other> operator|(Other) const noexcept {
        return {};
    }
};

template <char Min, char Max>
inline char_range_t<Min, Max> char_range = {};

inline auto HSPACE   = char_set<' ', '\t'>;
inline auto BIT      = char_set<'0', '1'>;
inline auto ANY_CHAR = char_range<'\x01', '\x7f'>;
inline auto ALPHA    = char_range<'A', 'Z'> | char_range<'a', 'z'>;
inline auto CTL      = char_range<'\x00', '\x1f'> | char_set<'\x7f'>;
inline auto DIGIT    = char_range<'0', '9'>;
inline auto DQUOTE   = char_set<'"'>;
inline auto HEXDIG   = DIGIT | char_range<'A', 'F'>;
inline auto HTAB     = char_set<'\t'>;
inline auto LF       = char_set<'\n'>;
inline auto SP       = char_set<' '>;
inline auto VCHAR    = char_range<'\x21', '\x7e'>;
inline auto WSP      = SP | HTAB;
inline auto token_char
    = char_set<'!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~'> | DIGIT
    | ALPHA;

inline auto UNRESERVED = ALPHA | DIGIT | char_set<'-', '.', '_', '~'>;
inline auto SUB_DELIMS = char_set<'!', '$', '&', '\'', '(', ')', '@', '*', '+', ',', ';', '='>;

// Path elements may also contain percent-encoded elements
inline auto PCHAR_BASIC_CHARS = UNRESERVED | SUB_DELIMS | char_set<':', '@'>;

//    reserved      = gen-delims / sub-delims
//    gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
//    sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
//                  / "*" / "+" / "," / ";" / "="

}  // namespace neo::http::parse_detail
