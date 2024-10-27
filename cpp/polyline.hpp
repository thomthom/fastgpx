#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fastgpx {
struct LatLong;

namespace polyline {

enum class Precision
{
  Five = 5,
  Six = 6
};

std::string encode(std::span<const LatLong> locations, Precision precision = Precision::Five);

std::vector<LatLong> decode(std::string_view encoded, Precision precision = Precision::Five);

} // namespace polyline

} // namespace fastgpx
