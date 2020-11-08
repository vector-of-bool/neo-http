#pragma once

#include "./message.hpp"
#include <neo/http/parse/header.hpp>
#include <neo/http/parse/status.hpp>

namespace neo::http {

struct response_head : message_head<response_head, status_line> {};

}  // namespace neo::http
