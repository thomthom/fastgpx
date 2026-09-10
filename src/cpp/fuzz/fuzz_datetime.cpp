// Fuzz target: fastgpx::parse_gpx_time (the parser used for <time> elements).
//
// An input with a NUL byte in it is also read as two timestamps, and checks the property
// `Segment::ComputeTimeBounds` relies on: for two strings of the same length that
// `is_sortable_gpx_time` accepts, comparing the strings gives the same answer as comparing the
// times they parse to. Inputs without a NUL exercise the parser alone, as they always did. A NUL
// is the separator because git leaves a file that contains one byte for byte, so the paired
// corpus seeds mean the same thing on every platform.

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "fastgpx/datetime.hpp"
#include "fastgpx/errors.hpp"

namespace {

// Parses `time_str`, or returns false if it is out of the range the platform can represent.
// A sortable string is well formed, so that is the only error it can raise.
bool TryParse(std::string_view time_str, std::chrono::system_clock::time_point& out)
{
  try
  {
    out = fastgpx::parse_gpx_time(time_str);
    return true;
  }
  catch (const fastgpx::parse_error&)
  {
    return false;
  }
}

void CheckSortableOrder(std::string_view lhs, std::string_view rhs)
{
  if (lhs.size() != rhs.size() || !fastgpx::is_sortable_gpx_time(lhs) ||
      !fastgpx::is_sortable_gpx_time(rhs))
  {
    return;
  }

  std::chrono::system_clock::time_point lhs_time;
  std::chrono::system_clock::time_point rhs_time;
  if (!TryParse(lhs, lhs_time) || !TryParse(rhs, rhs_time))
  {
    return;
  }

  assert((lhs < rhs) == (lhs_time < rhs_time));
  assert((lhs == rhs) == (lhs_time == rhs_time));
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
  const std::string_view input(reinterpret_cast<const char*>(data), size);

  try
  {
    (void)fastgpx::parse_gpx_time(input);
  }
  catch (const fastgpx::fastgpx_error&)
  {
    // Rejecting malformed input with the library's own error type is the expected behaviour.
    // Any other exception escapes and is reported by the fuzzer.
  }

  if (const auto split = input.find('\0'); split != std::string_view::npos)
  {
    CheckSortableOrder(input.substr(0, split), input.substr(split + 1));
  }

  return 0;
}
