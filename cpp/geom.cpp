#include "geom.hpp"

#include <cmath>
#include <numbers>

#include "fastgpx.hpp"

namespace fastgpx
{

  // latitude/longitude in GPX files is always in WGS84 datum
  // WGS84 defined the Earth semi-major axis with 6378.137 km
  constexpr double EARTH_RADIUS = 6378.137 * 1000.0;

  // One degree in meters:
  constexpr double ONE_DEGREE = (2.0 * std::numbers::pi * EARTH_RADIUS) / 360.0; // == > 111.319 km

  double radians(double degrees)
  {
    constexpr double TO_RADIANS = std::numbers::pi / 180.0;
    return degrees * TO_RADIANS;
  }

  /**
   * @brief Haversine distance returned in meters.
   *
   * @param ll1
   * @param ll2
   * @return double Meters.
   */
  double haversine(LatLong ll1, LatLong ll2)
  {
    const auto d_lon = radians(ll1.longitude - ll2.longitude);
    const auto lat1 = radians(ll1.latitude);
    const auto lat2 = radians(ll2.latitude);
    const auto d_lat = lat1 - lat2;

    const auto a = std::pow(std::sin(d_lat / 2), 2) +
                   std::pow(std::sin(d_lon / 2), 2) * std::cos(lat1) * std::cos(lat2);
    const auto c = 2 * std::asin(std::sqrt(a));
    const auto d = EARTH_RADIUS * c;
    return d;
  }

  // TODO: Compare against always using haversine.
  // TODO: Compare against libosmium.
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
  double distance(LatLong ll2, LatLong ll1, bool use_haversine = false, bool use_2d = true)
  {
    if (use_haversine || (std::abs(ll1.latitude - ll2.latitude) > .2 || std::abs(ll1.longitude - ll2.longitude) > .2))
      return haversine(ll1, ll2);

    const auto coef = std::cos(radians(ll1.latitude));
    const auto x = ll1.latitude - ll2.latitude;
    const auto y = (ll1.longitude - ll2.longitude) * coef;

    const auto distance_2d = std::sqrt(x * x + y * y) * ONE_DEGREE;

    // TODO: Make elevation into std::optional?
    // if (ll1.elevation is None || elevation_2 is None || ll1.elevation == elevation_2)
    //     return distance_2d
    if (use_2d || ll1.elevation == ll2.elevation)
      return distance_2d;

    return std::sqrt(std::pow(distance_2d, 2.0) + std::pow((ll1.elevation - ll2.elevation), 2.0));
  }

  double distance2d(LatLong ll1, LatLong ll2, bool use_haversine)
  {
    return distance(ll1, ll2, use_haversine, true);
  }

  double distance3d(LatLong ll1, LatLong ll2, bool use_haversine)
  {
    return distance(ll1, ll2, use_haversine, false);
  }

} // namespace fastgpx
