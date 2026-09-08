#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace fastgpx {

class fastgpx_error : public std::runtime_error
{
public:
  explicit fastgpx_error(const std::string& message) : std::runtime_error(message) {}
};

class parse_error : public fastgpx_error
{
public:
  explicit parse_error(const std::string& message) : fastgpx_error(message) {}
  parse_error(const std::string& message, std::string_view source_str, std::string_view sub_str);
};

// Thrown when a function is given a value it cannot operate on, e.g. a non-finite or out-of-range
// coordinate passed to `polyline::encode`. Surfaces as `ValueError` in Python.
class value_error : public fastgpx_error
{
public:
  explicit value_error(const std::string& message) : fastgpx_error(message) {}
};

} // namespace fastgpx
