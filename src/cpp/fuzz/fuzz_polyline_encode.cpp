// Fuzz target: fastgpx::polyline::encode, checked against decode.
//
// The input is interpreted as a precision selector byte followed by raw (latitude, longitude)
// double pairs, fed to the encoder unmodified. The encoder must either reject the input with a
// `value_error` - exactly when a coordinate is non-finite or rounds outside +/-90 / +/-180 - or
// encode it such that decoding gives back every coordinate at the selected precision.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "fastgpx/errors.hpp"
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

  // Mirror of the encoder's contract, computed independently.
  const auto representable = [factor](double value, double limit) {
    const double scaled = std::round(value * factor);
    return std::isfinite(scaled) && std::abs(scaled) <= limit * factor;
  };

  std::vector<fastgpx::LatLong> points;
  bool expect_valid = true;
  constexpr std::size_t kPairSize = 2 * sizeof(double);
  for (std::size_t offset = 0; offset + kPairSize <= size; offset += kPairSize)
  {
    double latitude = 0.0;
    double longitude = 0.0;
    std::memcpy(&latitude, data + offset, sizeof(double));
    std::memcpy(&longitude, data + offset + sizeof(double), sizeof(double));
    expect_valid = expect_valid && representable(latitude, 90.0) && representable(longitude, 180.0);
    points.push_back({latitude, longitude, 0.0});
  }

  std::string encoded;
  try
  {
    encoded = fastgpx::polyline::encode(points, precision);
  }
  catch (const fastgpx::value_error&)
  {
    if (expect_valid)
    {
      std::abort(); // Rejected input the contract says is representable.
    }
    return 0;
  }
  if (!expect_valid)
  {
    std::abort(); // Accepted input the contract says must be rejected.
  }

  // The encoder produces valid polylines for valid coordinates: decode must not throw.
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
