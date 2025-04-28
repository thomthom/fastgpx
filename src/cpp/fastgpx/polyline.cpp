#include "fastgpx/polyline.hpp"

#include <cmath>
#include <utility>

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
  int lat = 0;
  int lng = 0;

  while (index < encoded.size())
  {
    auto decode_value = [&](int& value) -> void {
      int shift = 0;
      int result = 0;
      int byte;

      do
      {
        byte = encoded[index++] - 63;
        result |= (byte & 0x1f) << shift;
        shift += 5;
      }
      while (byte >= 0x20);

      value += ((result & 1) ? ~(result >> 1) : (result >> 1));
    };

    decode_value(lat);
    decode_value(lng);

    coordinates.push_back(
        {static_cast<double>(lat) / factor, static_cast<double>(lng) / factor, 0.0});
  }

  return coordinates;
}

} // namespace polyline

} // namespace fastgpx
