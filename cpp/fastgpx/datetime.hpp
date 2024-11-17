#pragma once

#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fastgpx {

class fastgpx_error : public std::runtime_error
{
public:
  explicit fastgpx_error(const std::string &message) : std::runtime_error(message) {}
};

class parse_error : public fastgpx_error
{
public:
  explicit parse_error(const std::string &message) : fastgpx_error(message) {}
  parse_error(const std::string &message, std::string_view source_str, std::string_view sub_str);
};

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

namespace v5 {

std::chrono::system_clock::time_point parse_iso8601(std::string_view time_str);

} // namespace v5

using v3::parse_iso8601;

} // namespace fastgpx
