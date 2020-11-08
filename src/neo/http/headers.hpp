#pragma once

#include <neo/assert.hpp>
#include <neo/opt_ref.hpp>

#include <algorithm>
#include <memory>
#include <vector>

namespace neo::http {

constexpr bool header_key_equivalent(std::string_view key1, std::string_view key2) noexcept {
    if (key1.size() != key2.size()) {
        return false;
    }
    auto lower = [](char c) {
        if (c >= 'A' && c <= 'Z') {
            auto offset = c - 'A';
            return static_cast<char>('a' + offset);
        }
        return c;
    };
    for (auto it_1 = key1.cbegin(), it_2 = key2.cbegin(); it_1 != key1.cend(); ++it_1, ++it_2) {
        if (lower(*it_1) != lower(*it_2)) {
            return false;
        }
    }
    return true;
}

template <typename Allocator = std::allocator<void>>
class basic_headers {
public:
    using allocator_type = Allocator;
    using string_type    = std::basic_string<
        char,
        std::char_traits<char>,
        typename std::allocator_traits<allocator_type>::template rebind_alloc<char>>;

    struct header_item {
        string_type key;
        string_type value;

        bool key_equal(std::string_view other) const noexcept {
            return header_key_equivalent(key, other);
        }
    };

private:
    allocator_type _alloc{};
    std::vector<header_item,
                typename std::allocator_traits<allocator_type>::template rebind_alloc<header_item>>
        _vec{_alloc};

    template <typename Self>
    static auto _find(Self& self, std::string_view key) noexcept {
        return std::find_if(self.begin(), self.end(), [&](const header_item& cand) {
            return cand.key_equal(key);
        });
    }

public:
    basic_headers() = default;
    explicit basic_headers(allocator_type alloc) noexcept
        : _alloc(alloc) {}

    allocator_type get_allocator() const noexcept { return _alloc; }

    header_item& add(std::string_view key, std::string_view val) noexcept {
        return _vec.emplace_back(
            header_item{string_type{key, get_allocator()}, string_type{val, get_allocator()}});
    }

    using iterator       = header_item*;
    using const_iterator = const header_item*;
    using size_type      = std::size_t;

    size_type size() const noexcept { return _vec.size(); }

    auto begin() noexcept { return _vec.data(); }
    auto begin() const noexcept { return _vec.data(); }
    auto cbegin() const noexcept { return begin(); }
    auto end() noexcept { return begin() + size(); }
    auto end() const noexcept { return begin() + size(); }
    auto cend() const noexcept { return end; }

    opt_ref<header_item> find(std::string_view key) noexcept {
        auto found = _find(*this, key);
        return found == end() ? std::nullopt : opt_ref(*found);
    }

    opt_ref<const header_item> find(std::string_view key) const noexcept {
        auto found = _find(*this, key);
        return found == end() ? std::nullopt : opt_ref(*found);
    }

    header_item& operator[](std::string_view key) noexcept {
        auto found = find(key);
        neo_assert(expects, found != std::nullopt, "Request for non-existent header", key);
        return *found;
    }

    const header_item& operator[](std::string_view key) const noexcept {
        auto found = find(key);
        neo_assert(expects, found != std::nullopt, "Request for non-existent header", key);
        return *found;
    }
};

using headers = basic_headers<>;

}  // namespace neo::http