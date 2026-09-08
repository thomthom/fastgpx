#include "fastgpx/polyline.hpp"

#include <cmath>
#include <cstdint>
#include <utility>

#include "fastgpx/errors.hpp"
#include "fastgpx/fastgpx.hpp"

namespace fastgpx {

namespace polyline {

std::string encode(std::span<const LatLong> locations, Precision precision)
{
  const int factor = (precision == Precision::Six) ? 1'000'000 : 100'000;
  std::string encoded_polyline;
  int last_lat = 0;
  int last_lng = 0;

  for (const auto& coord : locations)
  {
    const int lat = static_cast<int>(std::round(coord.latitude * factor));
    const int lng = static_cast<int>(std::round(coord.longitude * factor));

    const int delta_lat = lat - last_lat;
    const int delta_lng = lng - last_lng;

    auto encode_value = [](int value) -> std::string {
      std::string encoded_latlong;
      value = (value < 0) ? ~(value << 1) : (value << 1);
      while (value >= 0x20)
      {
        encoded_latlong += static_cast<char>((0x20 | (value & 0x1f)) + 63);
        value >>= 5;
      }
      encoded_latlong += static_cast<char>(value + 63);
      return encoded_latlong;
    };

    encoded_polyline += encode_value(delta_lat);
    encoded_polyline += encode_value(delta_lng);

    last_lat = lat;
    last_lng = lng;
  }

  return encoded_polyline;
}

std::vector<LatLong> decode(std::string_view encoded, Precision precision)
{
  const int factor = (precision == Precision::Six) ? 1'000'000 : 100'000;
  std::vector<LatLong> coordinates;
  size_t index = 0;
  // Each delta can be up to ±2^29 in magnitude, so a sequence of deltas can overflow a 32-bit
  // accumulator (signed overflow is undefined behavior). Accumulate in 64-bit and reject values
  // that leave the valid coordinate range instead.
  std::int64_t lat = 0;
  std::int64_t lng = 0;
  const std::int64_t lat_limit = std::int64_t{90} * factor;
  const std::int64_t lng_limit = std::int64_t{180} * factor;

  auto decode_value = [&](std::int64_t& value, std::int64_t limit, const char* name) -> void {
    int shift = 0;
    std::int64_t result = 0;
    int byte = 0;

    do
    {
      if (index >= encoded.size())
      {
        throw parse_error("polyline: unexpected end of encoded data");
      }
      // Each chunk carries 5 bits. Values are 32-bit, so at most 6 chunks (30 bits) are valid.
      // A 7th chunk would shift past the width of `int`, which is undefined behavior.
      if (shift >= 30)
      {
        throw parse_error("polyline: encoded value is too long");
      }
      byte = static_cast<unsigned char>(encoded[index++]) - 63;
      if (byte < 0 || byte > 63)
      {
        throw parse_error("polyline: invalid character in encoded data");
      }
      result |= static_cast<std::int64_t>(byte & 0x1f) << shift;
      shift += 5;
    }
    while (byte >= 0x20);

    value += ((result & 1) ? ~(result >> 1) : (result >> 1));

    // Reject accumulated values outside the representable coordinate range for the precision
    // (|latitude| <= 90 and |longitude| <= 180). This is stricter than merely guarding the
    // integer range: such input cannot come from a valid encoder and would otherwise decode
    // into nonsensical coordinates.
    if (value < -limit || value > limit)
    {
      throw parse_error(std::string("polyline: ") + name + " out of range");
    }
  };

  while (index < encoded.size())
  {
    decode_value(lat, lat_limit, "latitude");
    decode_value(lng, lng_limit, "longitude");

    coordinates.push_back(
        {static_cast<double>(lat) / factor, static_cast<double>(lng) / factor, 0.0});
  }

  return coordinates;
}

} // namespace polyline

} // namespace fastgpx
