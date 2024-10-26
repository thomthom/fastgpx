#include "fastgpx.hpp"

#include <pugixml.hpp>

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

#include "filesystem.hpp"
#include "geom.hpp"

namespace fastgpx
{
  // Bounds

  void Bounds::Add(const LatLong &location)
  {
    min.latitude = std::min(min.latitude, location.latitude);
    min.longitude = std::min(min.longitude, location.longitude);
    max.latitude = std::max(max.latitude, location.latitude);
    max.longitude = std::max(max.longitude, location.longitude);
  }

  void Bounds::Add(std::span<const LatLong> locations)
  {
    for (const auto &location : locations)
    {
      Add(location);
    }
  }

  void Bounds::Add(const Bounds &bounds)
  {
    // TODO: compare all values? In case min/max is not initialized correctly.
    min.latitude = std::min(min.latitude, bounds.min.latitude);
    min.longitude = std::min(min.longitude, bounds.min.longitude);
    max.latitude = std::max(max.latitude, bounds.max.latitude);
    max.longitude = std::max(max.longitude, bounds.max.longitude);
  }

  Bounds Bounds::MaxBounds(const Bounds &bounds) const
  {
    Bounds max = *this;
    max.Add(bounds);
    return max;
  }

  // Segment

  const Bounds &Segment::GetBounds() const
  {
    if (!bounds.has_value())
    {
      bounds = ComputeBounds();
    }
    return bounds.value();
  }

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

  Bounds Segment::ComputeBounds() const
  {
    Bounds bounds;
    bounds.Add(points);
    return bounds;
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

  const Bounds &Track::GetBounds() const
  {
    if (!bounds.has_value())
    {
      bounds = ComputeBounds();
    }
    return bounds.value();
  }

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

  Bounds Track::ComputeBounds() const
  {
    Bounds bounds;
    for (const auto &segment : segments)
    {
      bounds.Add(segment.GetBounds());
    }
    return bounds;
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

  const Bounds &Gpx::GetBounds() const
  {
    if (!bounds.has_value())
    {
      bounds = ComputeBounds();
    }
    return bounds.value();
  }

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

  Bounds Gpx::ComputeBounds() const
  {
    Bounds bounds;
    for (const auto &track : tracks)
    {
      bounds.Add(track.GetBounds());
    }
    return bounds;
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

  Gpx ParseGpx(const std::string &path)
  {
    return ParseGpx(std::filesystem::path(path));
  }

} // namespace fastgpx
