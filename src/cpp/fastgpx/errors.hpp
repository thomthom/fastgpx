#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace fastgpx {

// Base of all fastgpx exceptions. Every subclass describes input the library cannot use, so in
// Python the hierarchy surfaces as `fastgpx.Error`, a `ValueError` subclass, except `file_error`
// which maps to the matching `OSError`.
class fastgpx_error : public std::runtime_error
{
public:
  explicit fastgpx_error(const std::string& message) : std::runtime_error(message) {}
};

// Thrown when GPX data, a polyline string or a timestamp is malformed. `fastgpx.ParseError` in
// Python.
class parse_error : public fastgpx_error
{
public:
  explicit parse_error(const std::string& message) : fastgpx_error(message) {}
  parse_error(const std::string& message, std::string_view source_str, std::string_view sub_str);
};

// Thrown when a function is given a value it cannot operate on, e.g. a non-finite or out-of-range
// coordinate passed to `polyline::encode`.
class value_error : public fastgpx_error
{
public:
  explicit value_error(const std::string& message) : fastgpx_error(message) {}
};

// Thrown when a file cannot be read. `not_found()` distinguishes a missing file from other I/O
// failures. `FileNotFoundError` or `OSError` in Python.
class file_error : public fastgpx_error
{
public:
  file_error(const std::string& message, bool not_found)
      : fastgpx_error(message), not_found_(not_found)
  {
  }

  bool not_found() const noexcept { return not_found_; }

private:
  bool not_found_;
};

} // namespace fastgpx
