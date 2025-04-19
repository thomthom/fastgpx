#include "fastgpx/geom.hpp"

#include <cmath>
#include <numbers>

#include "fastgpx/fastgpx.hpp"

namespace fastgpx {
namespace v1 {

// latitude/longitude in GPX files is always in WGS84 datum
// WGS84 defined the Earth semi-major axis with 6378.137 km
constexpr double EARTH_RADIUS = 6378.137 * 1000.0;

// One degree in meters:
constexpr double ONE_DEGREE = (2.0 * std::numbers::pi * EARTH_RADIUS) / 360.0; // == > 111.319 km

double to_radians(double degrees) noexcept
{
  constexpr double TO_RADIANS = std::numbers::pi / 180.0;
  return degrees * TO_RADIANS;
}

double haversine(const LatLong& ll1, const LatLong& ll2) noexcept
{
  const auto d_lon = to_radians(ll1.longitude - ll2.longitude);
  const auto lat1 = to_radians(ll1.latitude);
  const auto lat2 = to_radians(ll2.latitude);
  const auto d_lat = lat1 - lat2;

  const auto a = std::pow(std::sin(d_lat / 2), 2) +
                 std::pow(std::sin(d_lon / 2), 2) * std::cos(lat1) * std::cos(lat2);
  const auto c = 2 * std::asin(std::sqrt(a));
  const auto d = EARTH_RADIUS * c;
  return d;
}

/**
 * @brief Computes the distance between two points using gpxpy logic.
 *
 * Distance between two points. If elevation is None compute a 2d distance
 *
 * if haversine==True -- haversine will be used for every computations,
 * otherwise...
 *
 * Haversine distance will be used for distant points where elevation makes a
 * small difference, so it is ignored. That's because haversine is 5-6 times
 * slower than the dummy distance algorithm (which is OK for most GPS tracks).
 *
 * @note Flipping ll2 and ll1 order so the calculation matches gpxpy.
 *   If they are reversed there is a slight difference.
 *   https://github.com/tkrajina/gpxpy/blob/09fc46b3cad16b5bf49edf8e7ae873794a959620/gpxpy/geo.py#L95-L122
 *
 * @param ll2
 * @param ll1
 * @param use_haversine
 * @param use_2d
 * @return double Meters
 */
double distance(const LatLong& ll2, const LatLong& ll1, bool use_haversine = false,
                bool use_2d = true) noexcept
{
  if (use_haversine ||
      (std::abs(ll1.latitude - ll2.latitude) > .2 || std::abs(ll1.longitude - ll2.longitude) > .2))
    return v1::haversine(ll1, ll2);

  const auto coef = std::cos(to_radians(ll1.latitude));
  const auto x = ll1.latitude - ll2.latitude;
  const auto y = (ll1.longitude - ll2.longitude) * coef;

  const auto distance_2d = std::sqrt(x * x + y * y) * ONE_DEGREE;

  // TODO: Make elevation into std::optional?
  // if (ll1.elevation is None || elevation_2 is None || ll1.elevation == elevation_2)
  //     return distance_2d
  if (use_2d || ll1.elevation == ll2.elevation)
    return distance_2d;

  // return std::sqrt(std::pow(distance_2d, 2.0) + std::pow((ll1.elevation - ll2.elevation), 2.0));
  const auto ele_diff = ll1.elevation - ll2.elevation;
  return std::sqrt((distance_2d * distance_2d) + (ele_diff * ele_diff));
}

double distance2d(const LatLong& ll1, const LatLong& ll2, bool use_haversine) noexcept
{
  return v1::distance(ll1, ll2, use_haversine, true);
}

double distance3d(const LatLong& ll1, const LatLong& ll2, bool use_haversine) noexcept
{
  return v1::distance(ll1, ll2, use_haversine, false);
}

} // namespace v1

namespace v2 {

namespace geom {

// constexpr inline double std::numbers::pi = (3.141592653589793)
constexpr double PI = 3.14159265358979323846;

/// Convert angle from degrees to radians.
inline constexpr double deg_to_rad(double degree) noexcept
{
  return degree * (PI / 180.0);
}

/// Convert angle from radians to degrees.
inline constexpr double rad_to_deg(double radians) noexcept
{
  return radians * (180.0 / PI);
}

} // namespace geom

/// @brief Earth's quadratic mean radius for WGS84
constexpr const double EARTH_RADIUS_IN_METERS = 6372797.560856;

double haversine(const LatLong& ll1, const LatLong& ll2) noexcept
{
  using namespace geom;
  // https://github.com/osmcode/libosmium/blob/f88048769c13210ca81efca17668dc57ea64c632/include/osmium/geom/haversine.hpp#L48-L73
  double lon = std::sin(deg_to_rad(ll1.longitude - ll2.longitude) * 0.5);
  lon *= lon;

  double lat = std::sin(deg_to_rad(ll1.latitude - ll2.latitude) * 0.5);
  lat *= lat;

  const double tmp = std::cos(deg_to_rad(ll1.latitude)) * std::cos(deg_to_rad(ll2.latitude));
  return 2.0 * EARTH_RADIUS_IN_METERS * std::asin(std::sqrt(lat + tmp * lon));
}

double distance2d(const LatLong& ll1, const LatLong& ll2) noexcept
{
  return v2::haversine(ll1, ll2);
}

double distance3d(const LatLong& ll1, const LatLong& ll2) noexcept
{
  const auto distance = v2::haversine(ll1, ll2);

  const auto elevation_diff = ll1.elevation - ll2.elevation;
  return std::sqrt((distance * distance) + (elevation_diff * elevation_diff));
}
} // namespace v2

} // namespace fastgpx
