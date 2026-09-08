// Fuzz target: fastgpx::parse_gpx_time (the v6 parser used for <time> elements).

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "fastgpx/datetime.hpp"
#include "fastgpx/errors.hpp"

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

  return 0;
}
