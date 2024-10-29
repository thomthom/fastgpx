#pragma once

#include <chrono>
#include <string>

namespace fastgpx {

namespace v1 {

std::chrono::system_clock::time_point parse_iso8601(const std::string &time_str);

} // namespace v1

using v1::parse_iso8601;

} // namespace fastgpx
