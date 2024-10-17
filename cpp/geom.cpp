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

  /*
    """
    Haversine distance between two points, expressed in meters.

    Implemented from http://www.movable-type.co.uk/scripts/latlong.html
    """
    d_lon = mod_math.radians(longitude_1 - longitude_2)
    lat1 = mod_math.radians(latitude_1)
    lat2 = mod_math.radians(latitude_2)
    d_lat = lat1 - lat2

    a = mod_math.pow(mod_math.sin(d_lat/2),2) + \
        mod_math.pow(mod_math.sin(d_lon/2),2) * mod_math.cos(lat1) * mod_math.cos(lat2)
    c = 2 * mod_math.asin(mod_math.sqrt(a))
    d = EARTH_RADIUS * c
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

  // TODO: Return strong Meter type.
  /*
  double haversine(LatLong ll1, LatLong ll2)
  {
    const double R = 6371e3; // Earth radius in meters
    const double to_radians = std::numbers::pi / 180.0;

    auto lat1 = ll1.latitude;
    auto lon1 = ll1.longitude;
    auto lat2 = ll2.latitude;
    auto lon2 = ll2.longitude;

    lat1 *= to_radians;
    lon1 *= to_radians;
    lat2 *= to_radians;
    lon2 *= to_radians;

    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double a = sin(dlat / 2) * sin(dlat / 2) +
               cos(lat1) * cos(lat2) * sin(dlon / 2) * sin(dlon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return R * c; // Distance in meters
  }
  */

  /*
    """
    Distance between two points. If elevation is None compute a 2d distance

    if haversine==True -- haversine will be used for every computations,
    otherwise...

    Haversine distance will be used for distant points where elevation makes a
    small difference, so it is ignored. That's because haversine is 5-6 times
    slower than the dummy distance algorithm (which is OK for most GPS tracks).
    """

    # If points too distant -- compute haversine distance:
    if haversine or (abs(latitude_1 - latitude_2) > .2 or abs(longitude_1 - longitude_2) > .2):
        return haversine_distance(latitude_1, longitude_1, latitude_2, longitude_2)

    coef = mod_math.cos(mod_math.radians(latitude_1))
    x = latitude_1 - latitude_2
    y = (longitude_1 - longitude_2) * coef

    distance_2d = mod_math.sqrt(x * x + y * y) * ONE_DEGREE

    if elevation_1 is None or elevation_2 is None or elevation_1 == elevation_2:
        return distance_2d

    return mod_math.sqrt(distance_2d ** 2 + (elevation_1 - elevation_2) ** 2)
  */
  double distance(LatLong ll1, LatLong ll2, bool use_haversine = false, bool use_2d = true)
  {
    // If points too distant -- compute haversine distance:
    if (use_haversine || (std::abs(ll1.latitude - ll2.latitude) > .2 || std::abs(ll1.longitude - ll2.longitude) > .2))
      return haversine(ll1, ll2);

    const auto coef = std::cos(radians(ll1.latitude));
    const auto x = ll1.latitude - ll2.latitude;
    const auto y = (ll1.longitude - ll2.longitude) * coef;

    const auto distance_2d = std::sqrt(x * x + y * y) * ONE_DEGREE;

    // if (ll1.elevation is None || elevation_2 is None || ll1.elevation == elevation_2)
    //     return distance_2d
    if (use_2d || ll1.elevation == ll2.elevation)
      return distance_2d;

    // return std::sqrt(distance_2d ** 2.0 + (ll1.elevation - ll2.elevation) ** 2.0);
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
