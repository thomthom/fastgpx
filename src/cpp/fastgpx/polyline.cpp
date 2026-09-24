#include "fastgpx/polyline.hpp"

#include <cmath>
#include <cstdint>
#include <format>
#include <utility>

#include "fastgpx/errors.hpp"
#include "fastgpx/fastgpx.hpp"

namespace fastgpx {

namespace polyline {

std::string encode(std::span<const LatLong> locations, Precision precision)
{
  const int factor = (precision == Precision::Six) ? 1'000'000 : 100'000;
  // Same limits as `decode`: |latitude| <= 90 and |longitude| <= 180 after rounding.
  const double lat_limit = 90.0 * factor;
  const double lng_limit = 180.0 * factor;

  // Rounds a coordinate to the precision and rejects what cannot be encoded. The check is done on
  // the double: casting NaN, infinity or an out-of-range value to `int` is undefined behaviour
  // (found while writing fuzz_polyline_encode, #49). `!(x <= limit)` is also true for NaN.
  const auto to_fixed = [factor](double value, double limit, const char* name) -> int {
    const double scaled = std::round(value * factor);
    if (!(std::abs(scaled) <= limit))
    {
      throw value_error(std::format("polyline: {} out of range: {}", name, value));
    }
    return static_cast<int>(scaled);
  };

  std::string encoded_polyline;
  int last_lat = 0;
  int last_lng = 0;

  // Appends straight to the result rather than building a temporary string per value. With the
  // temporary inlined, MSVC read its pointer field right after writing characters into the same
  // bytes (they share storage while the string is short), which stalled on every character and
  // made encoding 2-3x slower under link-time optimization. See benchmarks/build_settings.md.
  //
  // `value` is the difference to the previous point in fixed-point units (1e-5 or 1e-6 degrees).
  // The encoding is Google's: the value is written as 5-bit chunks, least significant first, each
  // turned into one printable character.
  const auto append_value = [&encoded_polyline](int value) {
    // Move the sign into the lowest bit: shift left one, and for a negative value invert all bits,
    // so -1 becomes 1, 1 becomes 2, -2 becomes 3 and so on. Small values of either sign then need
    // few chunks. The result is never negative, so the shifts below see no sign bit.
    value = (value < 0) ? ~(value << 1) : (value << 1);
    // Loop while the value needs more than five bits.
    while (value >= 0x20)
    {
      // The low five bits, with bit 0x20 set to tell the decoder another chunk follows. Adding 63
      // turns the 0-63 chunk into a printable character, '?' to '~'.
      encoded_polyline.push_back(static_cast<char>((0x20 | (value & 0x1f)) + 63));
      // Drop the five bits just written.
      value >>= 5;
    }
    // The last chunk: at most five bits, no continuation mark.
    encoded_polyline.push_back(static_cast<char>(value + 63));
  };

  for (const auto& coord : locations)
  {
    const int lat = to_fixed(coord.latitude, lat_limit, "latitude");
    const int lng = to_fixed(coord.longitude, lng_limit, "longitude");

    append_value(lat - last_lat);
    append_value(lng - last_lng);

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
