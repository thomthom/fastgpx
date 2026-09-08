// Fuzz target: fastgpx::polyline::encode, checked against decode.
//
// The input is interpreted as a precision selector byte followed by raw (latitude, longitude)
// double pairs. Encoding must succeed and decoding the result must give back every coordinate,
// rounded to the selected precision.
//
// The encoder casts `std::round(coordinate * factor)` to `int`. Non-finite values and values
// outside roughly +/-21474 degrees make that cast undefined behaviour, which UBSan reports
// immediately. The values are therefore folded into the valid coordinate range here so that the
// fuzzer can explore the rest of the encoder. Keep that limitation in mind: `encode` itself does
// not validate its input.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "fastgpx/fastgpx.hpp"
#include "fastgpx/polyline.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
  if (size == 0)
  {
    return 0;
  }

  const auto precision =
      (data[0] & 1) ? fastgpx::polyline::Precision::Six : fastgpx::polyline::Precision::Five;
  const double factor = (precision == fastgpx::polyline::Precision::Six) ? 1e6 : 1e5;
  ++data;
  --size;

  std::vector<fastgpx::LatLong> points;
  constexpr std::size_t kPairSize = 2 * sizeof(double);
  for (std::size_t offset = 0; offset + kPairSize <= size; offset += kPairSize)
  {
    double latitude = 0.0;
    double longitude = 0.0;
    std::memcpy(&latitude, data + offset, sizeof(double));
    std::memcpy(&longitude, data + offset + sizeof(double), sizeof(double));
    if (!std::isfinite(latitude) || !std::isfinite(longitude))
    {
      continue;
    }
    // Fold into (-90, 90) and (-180, 180). See the note at the top of the file.
    latitude = std::fmod(latitude, 90.0);
    longitude = std::fmod(longitude, 180.0);
    points.push_back({latitude, longitude, 0.0});
  }

  // Neither call may throw: the encoder produces valid polylines for valid coordinates.
  const std::string encoded = fastgpx::polyline::encode(points, precision);
  const std::vector<fastgpx::LatLong> decoded = fastgpx::polyline::decode(encoded, precision);

  if (decoded.size() != points.size())
  {
    std::abort();
  }
  for (std::size_t i = 0; i < points.size(); ++i)
  {
    if (std::llround(points[i].latitude * factor) != std::llround(decoded[i].latitude * factor) ||
        std::llround(points[i].longitude * factor) != std::llround(decoded[i].longitude * factor))
    {
      std::abort();
    }
  }

  return 0;
}
