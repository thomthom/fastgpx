#include "fastgpx.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <numbers>
#include <numeric>
#include <print>
#include <ranges>
#include <string>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h> // for _wfopen

static std::wstring utf8_to_utf16(const std::string &utf8_str)
{
  if (utf8_str.empty())
  {
    return std::wstring(); // Return an empty wide string if input is empty
  }

  // Step 1: Determine the size of the wide string buffer required
  int wide_char_count = MultiByteToWideChar(
      CP_UTF8,                           // Source string is in UTF-8
      0,                                 // No special flags
      utf8_str.c_str(),                  // Input UTF-8 string
      static_cast<int>(utf8_str.size()), // Length of the input string
      nullptr,                           // No output buffer yet
      0                                  // Request size of the output buffer
  );

  if (wide_char_count == 0)
  {
    throw std::runtime_error("Error converting UTF-8 to UTF-16: MultiByteToWideChar failed.");
  }

  // Step 2: Allocate the necessary buffer
  std::wstring utf16_str(wide_char_count, 0);

  // Step 3: Perform the conversion
  int result = MultiByteToWideChar(
      CP_UTF8,                           // Source string is in UTF-8
      0,                                 // No special flags
      utf8_str.c_str(),                  // Input UTF-8 string
      static_cast<int>(utf8_str.size()), // Length of the input string
      &utf16_str[0],                     // Output buffer for the wide string
      wide_char_count                    // Size of the output buffer
  );

  if (result == 0)
  {
    throw std::runtime_error("Error converting UTF-8 to UTF-16: MultiByteToWideChar failed.");
  }

  return utf16_str;
}
#endif

namespace fastgpx
{

  FILE *open_file(const std::filesystem::path &file_path)
  {
    FILE *file = nullptr;

#ifdef _WIN32
    // On Windows, open with wide string support
    const auto pathu16 = utf8_to_utf16(file_path.string());
    file = _wfopen(pathu16.c_str(), L"rb"); // Open for reading in binary mode
                                            // file = _wfopen(file_path.c_str(), L"rb"); // Open for reading in binary mode
#else
    // On other platforms, open with standard fopen
    file = fopen(file_path.string().c_str(), "rb"); // Open for reading in binary mode
#endif

    return file;
  }

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

  double distance2d(LatLong ll1, LatLong ll2, bool use_haversine = false)
  {
    return distance(ll1, ll2, use_haversine, false);
  }

  double distance3d(LatLong ll1, LatLong ll2, bool use_haversine = false)
  {
    return distance(ll1, ll2, use_haversine, true);
  }

  // Segment

  double Segment::GetLength2D() const
  {
    if (!length2D.has_value())
    {
      length2D = ComputeLength2D();
    }
    return length2D.value();
  }

  double Segment::GetLength3D() const
  {
    if (!length3D.has_value())
    {
      length3D = ComputeLength3D();
    }
    return length3D.value();
  }

  double Segment::ComputeLength2D() const
  {
    auto distances = std::views::zip(points, points | std::views::drop(1)) |
                     std::views::transform([](const auto &pair)
                                           { return distance2d(std::get<0>(pair), std::get<1>(pair)); });
    return std::accumulate(distances.begin(), distances.end(), 0.0);
  }

  double Segment::ComputeLength3D() const
  {
    auto distances = std::views::zip(points, points | std::views::drop(1)) |
                     std::views::transform([](const auto &pair)
                                           { return distance3d(std::get<0>(pair), std::get<1>(pair)); });
    return std::accumulate(distances.begin(), distances.end(), 0.0);
  }

  // Track

  double Track::GetLength2D() const
  {
    if (!length2D.has_value())
    {
      length2D = ComputeLength2D();
    }
    return length2D.value();
  }

  double Track::GetLength3D() const
  {
    if (!length3D.has_value())
    {
      length3D = ComputeLength3D();
    }
    return length3D.value();
  }

  double Track::ComputeLength2D() const
  {
    return std::accumulate(segments.cbegin(), segments.cend(), 0.0, [](double acc, const Segment &segment)
                           { return acc + segment.GetLength2D(); });
  }

  double Track::ComputeLength3D() const
  {
    return std::accumulate(segments.cbegin(), segments.cend(), 0.0, [](double acc, const Segment &segment)
                           { return acc + segment.GetLength3D(); });
  }

  // Gpx

  double Gpx::GetLength2D() const
  {
    if (!length2D.has_value())
    {
      length2D = ComputeLength2D();
    }
    return length2D.value();
  }

  double Gpx::GetLength3D() const
  {
    if (!length3D.has_value())
    {
      length3D = ComputeLength3D();
    }
    return length3D.value();
  }

  double Gpx::ComputeLength2D() const
  {
    return std::accumulate(tracks.cbegin(), tracks.cend(), 0.0, [](double acc, const Track &track)
                           { return acc + track.GetLength2D(); });
  }

  double Gpx::ComputeLength3D() const
  {
    return std::accumulate(tracks.cbegin(), tracks.cend(), 0.0, [](double acc, const Track &track)
                           { return acc + track.GetLength3D(); });
  }

  Gpx ParseGpx(const std::filesystem::path &path)
  {
    pugi::xml_document doc;

    const auto path16 = utf8_to_utf16(path.string());
    pugi::xml_parse_result result = doc.load_file(path16.c_str());
    if (!result)
    {
      // std::cerr << "Failed to load GPX file: " << result.description() << "\n";
      std::println("Failed to load GPX file: {} - {}", result.description(), path.string());
      throw "Failed to load GPX file";
    }

    pugi::xml_node root = doc.child("gpx");
    Gpx gpx;

    // Iterate over each <trk> element
    for (pugi::xml_node track = root.child("trk"); track; track = track.next_sibling("trk"))
    {
      gpx.tracks.push_back({});
      auto &gpx_track = gpx.tracks.back();

      // Iterate over each <trkseg> element
      for (pugi::xml_node segment = track.child("trkseg"); segment; segment = segment.next_sibling("trkseg"))
      {
        gpx_track.segments.push_back({});
        auto &gpx_segment = gpx_track.segments.back();

        pugi::xml_node prev_trkpt;
        // Iterate over each <trkpt> in the segment
        for (pugi::xml_node trkpt = segment.child("trkpt"); trkpt; trkpt = trkpt.next_sibling("trkpt"))
        {
          const double lat = trkpt.attribute("lat").as_double();
          const double lon = trkpt.attribute("lon").as_double();

          // <ele>
          double elevation = 0.0;
          const auto ele = trkpt.child("ele");
          if (ele)
          {
            elevation = ele.text().as_double();
          }

          gpx_segment.points.emplace_back(lat, lon, elevation);
        }
      }
    }

    return gpx;
  }

} // namespace fastgpx
