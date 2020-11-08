#pragma once

#include <neo/http/version.hpp>

#include <neo/const_buffer.hpp>

namespace neo::http {

constexpr std::size_t version_buf_size = 8;  // std::strlen("HTTP/1.x");

version        parse_version(const_buffer) noexcept;
const_buffer   version_buf(version) noexcept;
mutable_buffer write_version(mutable_buffer, version) noexcept;

}  // namespace neo::http