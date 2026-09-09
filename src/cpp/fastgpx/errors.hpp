#pragma once

#include <cstddef>
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

  // Composes a message that quotes `source_str` and marks `length` characters from `offset` with
  // a caret line. The marker takes an index rather than a sub-view of `source_str`, because
  // deriving one from a view that does not point into `source_str` would be undefined behaviour.
  // An `offset` or `length` reaching past the end of `source_str` is clamped to it.
  parse_error(const std::string& message, std::string_view source_str, std::size_t offset,
              std::size_t length);
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
