#pragma once

#include <chrono>
#include <string>
#include <string_view>

namespace fastgpx {

namespace v1 {

std::chrono::system_clock::time_point parse_iso8601(const std::string &time_str);

} // namespace v1

namespace v2 {

std::chrono::utc_clock::time_point parse_iso8601(const std::string &time_str);

} // namespace v2

namespace v3 {

std::chrono::system_clock::time_point parse_iso8601(const std::string &time_str);

} // namespace v3

namespace v4 {

std::chrono::system_clock::time_point parse_iso8601(std::string_view time_str);

} // namespace v4

using v3::parse_iso8601;

} // namespace fastgpx
