#include "fastgpx/fastgpx.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <format>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <numeric>
#include <print>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>

#include "fastgpx/datetime.hpp"
#include "fastgpx/errors.hpp"
#include "fastgpx/filesystem.hpp"
#include "fastgpx/geom.hpp"

namespace fastgpx {

// TimePoint

std::chrono::system_clock::time_point TimePoint::value() const
{
  if (std::holds_alternative<std::string>(data_))
  {
    const auto& time_string = std::get<std::string>(data_);
    data_ = parse_gpx_time(time_string);
  }
  assert(std::holds_alternative<std::chrono::system_clock::time_point>(data_));
  return std::get<std::chrono::system_clock::time_point>(data_);
}

// TimeBounds

bool TimeBounds::IsEmpty() const
{
  assert(start_time.has_value() == end_time.has_value());
  return !start_time.has_value() && !end_time.has_value();
}

void TimeBounds::Add(const std::chrono::system_clock::time_point time_point)
{
  if (start_time.has_value())
  {
    start_time = std::min(*start_time, time_point);
  }
  else
  {
    start_time = time_point;
  }

  if (end_time.has_value())
  {
    end_time = std::max(*end_time, time_point);
  }
  else
  {
    end_time = time_point;
  }
}

void TimeBounds::Add(const TimeBounds& bounds)
{
  if (bounds.start_time.has_value())
  {
    Add(*bounds.start_time);
  }
  if (bounds.end_time.has_value())
  {
    Add(*bounds.end_time);
  }
}

// Bounds

bool Bounds::IsEmpty() const
{
  assert(min.has_value() == max.has_value());
  return !min.has_value() && !max.has_value();
}

void Bounds::Add(const LatLong& location)
{
  // TODO: compare all values? In case min/max is not initialized correctly.
  if (min.has_value())
  {
    min->latitude = std::min(min->latitude, location.latitude);
    min->longitude = std::min(min->longitude, location.longitude);
  }
  else
  {
    min = location;
  }

  if (max.has_value())
  {
    max->latitude = std::max(max->latitude, location.latitude);
    max->longitude = std::max(max->longitude, location.longitude);
  }
  else
  {
    max = location;
  }
}

void Bounds::Add(std::span<const LatLong> locations)
{
  for (const auto& location : locations)
  {
    Add(location);
  }
}

void Bounds::Add(const Bounds& bounds)
{
  if (bounds.min.has_value())
  {
    Add(*bounds.min);
  }
  if (bounds.max.has_value())
  {
    Add(*bounds.max);
  }
}

Bounds Bounds::MaxBounds(const Bounds& bounds) const
{
  Bounds computed_max = *this;
  computed_max.Add(bounds);
  return computed_max;
}

// Segment

const Bounds& Segment::GetBounds() const
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

const TimeBounds& Segment::GetTimeBounds() const
{
  if (!time_bounds.has_value())
  {
    time_bounds = ComputeTimeBounds();
  }
  return time_bounds.value();
}

Bounds Segment::ComputeBounds() const
{
  Bounds computed_bounds;
  computed_bounds.Add(points);
  return computed_bounds;
}

double Segment::ComputeLength2D() const
{
  auto distances = std::views::zip(points, points | std::views::drop(1)) |
                   std::views::transform([](const auto& pair) {
                     return distance2d(std::get<0>(pair), std::get<1>(pair));
                   });
  return std::accumulate(distances.begin(), distances.end(), 0.0);
}

double Segment::ComputeLength3D() const
{
  auto distances = std::views::zip(points, points | std::views::drop(1)) |
                   std::views::transform([](const auto& pair) {
                     return distance3d(std::get<0>(pair), std::get<1>(pair));
                   });
  return std::accumulate(distances.begin(), distances.end(), 0.0);
}

TimeBounds Segment::ComputeTimeBounds() const
{
  TimeBounds computed_bounds;
  for (const auto& point : points)
  {
    if (point.time.has_value())
    {
      computed_bounds.Add(point.time->value());
    }
  }
  return computed_bounds;
}

// Track

const Bounds& Track::GetBounds() const
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

const TimeBounds& Track::GetTimeBounds() const
{
  if (!time_bounds.has_value())
  {
    time_bounds = ComputeTimeBounds();
  }
  return time_bounds.value();
}

Bounds Track::ComputeBounds() const
{
  Bounds computed_bounds;
  for (const auto& segment : segments)
  {
    computed_bounds.Add(segment.GetBounds());
  }
  return computed_bounds;
}

double Track::ComputeLength2D() const
{
  return std::accumulate(
      segments.cbegin(), segments.cend(), 0.0,
      [](double acc, const Segment& segment) { return acc + segment.GetLength2D(); });
}

double Track::ComputeLength3D() const
{
  return std::accumulate(
      segments.cbegin(), segments.cend(), 0.0,
      [](double acc, const Segment& segment) { return acc + segment.GetLength3D(); });
}

TimeBounds Track::ComputeTimeBounds() const
{
  TimeBounds computed_bounds;
  for (const auto& segment : segments)
  {
    computed_bounds.Add(segment.GetTimeBounds());
  }
  return computed_bounds;
}

// Gpx

const Bounds& Gpx::GetBounds() const
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

const TimeBounds& Gpx::GetTimeBounds() const
{
  if (!time_bounds.has_value())
  {
    time_bounds = ComputeTimeBounds();
  }
  return time_bounds.value();
}

Bounds Gpx::ComputeBounds() const
{
  Bounds computed_bounds;
  for (const auto& track : tracks)
  {
    computed_bounds.Add(track.GetBounds());
  }
  return computed_bounds;
}

double Gpx::ComputeLength2D() const
{
  return std::accumulate(tracks.cbegin(), tracks.cend(), 0.0,
                         [](double acc, const Track& track) { return acc + track.GetLength2D(); });
}

double Gpx::ComputeLength3D() const
{
  return std::accumulate(tracks.cbegin(), tracks.cend(), 0.0,
                         [](double acc, const Track& track) { return acc + track.GetLength3D(); });
}

TimeBounds Gpx::ComputeTimeBounds() const
{
  TimeBounds computed_bounds;
  for (const auto& track : tracks)
  {
    computed_bounds.Add(track.GetTimeBounds());
  }
  return computed_bounds;
}

Gpx ParseGpx(const std::filesystem::path& path)
{
  pugi::xml_document doc;

#ifdef _WIN32
  // On Windows, convert the path to UTF-16 for compatibility in order to load files
  // with non-ASCII characters in the path.
  const auto path16 = utf8_to_utf16(path.string());
  pugi::xml_parse_result result = doc.load_file(path16.c_str());
#else
  pugi::xml_parse_result result = doc.load_file(path.string().c_str());
#endif

  if (!result)
  {
    const auto message =
        std::format("Failed to load GPX file: {} - {}", result.description(), path.string());
    throw parse_error(message);
  }

  pugi::xml_node root = doc.child("gpx");
  Gpx gpx;

  // Iterate over each <trk> element
  for (pugi::xml_node track = root.child("trk"); track; track = track.next_sibling("trk"))
  {
    gpx.tracks.push_back({});
    auto& gpx_track = gpx.tracks.back();

    // Iterate over each <trkseg> element
    for (pugi::xml_node segment = track.child("trkseg"); segment;
         segment = segment.next_sibling("trkseg"))
    {
      gpx_track.segments.push_back({});
      auto& gpx_segment = gpx_track.segments.back();

      pugi::xml_node prev_trkpt;
      // Iterate over each <trkpt> in the segment
      for (pugi::xml_node trkpt = segment.child("trkpt"); trkpt;
           trkpt = trkpt.next_sibling("trkpt"))
      {
        const double lat = trkpt.attribute("lat").as_double();
        const double lon = trkpt.attribute("lon").as_double();

        // <ele>
        /*
        Elevation (in meters) of the point.
        */
        double elevation = 0.0;
        const auto ele = trkpt.child("ele");
        if (ele)
        {
          elevation = ele.text().as_double();
        }

        auto& point = gpx_segment.points.emplace_back(lat, lon, elevation);

        // TODO: Add parse options, selectively choose what to parse.
        // <time>
        /*
        Creation/modification timestamp for element. Date and time in are in Univeral Coordinated
        Time (UTC), not local time! Conforms to ISO 8601 specification for date/time representation.
        Fractional seconds are allowed for millisecond timing in tracklogs.
        */
        const auto time = trkpt.child("time");
        if (time)
        {
          // Read only the raw string, but don't parse it. This is done on demand
          // when the value is read.
          point.time = std::string(time.text().as_string());
        }
      }
    }
  }

  return gpx;
}

Gpx ParseGpx(const std::string& path)
{
  return ParseGpx(std::filesystem::path(path));
}

} // namespace fastgpx
