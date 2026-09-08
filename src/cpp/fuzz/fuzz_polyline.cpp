// Fuzz target: fastgpx::polyline::decode, plus an encode/decode round trip.
//
// Any input that decodes successfully is re-encoded and decoded again. The encoder's output is
// by definition valid, so the second decode must not throw, and it must yield the same
// coordinates. Violations abort so the fuzzer reports them.

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "fastgpx/errors.hpp"
#include "fastgpx/fastgpx.hpp"
#include "fastgpx/polyline.hpp"

namespace {

void CheckDecode(std::string_view encoded, fastgpx::polyline::Precision precision)
{
  std::vector<fastgpx::LatLong> decoded;
  try
  {
    decoded = fastgpx::polyline::decode(encoded, precision);
  }
  catch (const fastgpx::fastgpx_error&)
  {
    // Rejecting malformed input with the library's own error type is the expected behaviour.
    return;
  }

  // Round trip. Neither call may throw.
  const std::string re_encoded = fastgpx::polyline::encode(decoded, precision);
  const std::vector<fastgpx::LatLong> re_decoded =
      fastgpx::polyline::decode(re_encoded, precision);
  if (re_decoded != decoded)
  {
    std::abort();
  }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
  const std::string_view input(reinterpret_cast<const char*>(data), size);
  CheckDecode(input, fastgpx::polyline::Precision::Five);
  CheckDecode(input, fastgpx::polyline::Precision::Six);
  return 0;
}
