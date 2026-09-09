#include "fastgpx/fastgpx.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

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
  return !start_time.has_value() && !end_time.has_value();
}

bool TimeBounds::IsRange() const
{
  return start_time.has_value() && end_time.has_value();
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

namespace {

// pugixml's `as_double()` uses `strtod`, which honors the process' LC_NUMERIC locale. A host
// application that has called `setlocale` (e.g. to "de_DE") would then parse "61.5" as 61.
// `std::from_chars` is locale independent and considerably faster.
double ParseDouble(std::string_view text)
{
  // Unlike `strtod`, `std::from_chars` neither skips leading whitespace nor accepts a leading '+'.
  const auto first = text.find_first_not_of(" \t\n\r");
  if (first == std::string_view::npos)
  {
    return 0.0;
  }
  text.remove_prefix(first);
  if (text.front() == '+')
  {
    text.remove_prefix(1);
  }

#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 200000
  // libc++ only implements floating-point `std::from_chars` from version 20. Fall back to `strtod`
  // on older libc++. Note that this keeps the old locale-dependent behaviour there.
  const std::string buffer(text);
  char* end = nullptr;
  errno = 0;
  const double value = std::strtod(buffer.c_str(), &end);
  if (end == buffer.c_str() || errno == ERANGE)
  {
    // Invalid input and out-of-range values are treated alike, as with `std::from_chars` below.
    return 0.0;
  }
  return value;
#else
  double value = 0.0;
  const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{})
  {
    // Invalid input (`invalid_argument`) and out-of-range values (`result_out_of_range`) are
    // both treated as 0.0. `value` is left unmodified in either case.
    return 0.0;
  }
  return value;
#endif
}

Gpx ReadGpxXml(const pugi::xml_node& doc)
{
  Gpx gpx;

  pugi::xml_node root = doc.child("gpx");

  const auto metadata = root.child("metadata");
  if (metadata)
  {
    const auto name = metadata.child("name");
    if (name)
    {
      gpx.name.emplace(name.text().as_string());
    }
  }

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
        const double lat = ParseDouble(trkpt.attribute("lat").value());
        const double lon = ParseDouble(trkpt.attribute("lon").value());

        // <ele>
        /*
        Elevation (in meters) of the point.
        */
        double elevation = 0.0;
        const auto ele = trkpt.child("ele");
        if (ele)
        {
          elevation = ParseDouble(ele.text().get());
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
          point.time.emplace(std::string(time.text().as_string()));
        }
      }
    }
  }

  return gpx;
}

} // namespace

Gpx LoadGpx(const std::filesystem::path& path)
{
  pugi::xml_document doc;

#ifdef _WIN32
  pugi::xml_parse_result result = doc.load_file(path.wstring().c_str());
#else
  pugi::xml_parse_result result = doc.load_file(path.string().c_str());
#endif

  if (!result)
  {
    const auto message =
        std::format("Failed to load GPX file: {} - {}", result.description(), path.string());
    if (result.status == pugi::status_file_not_found || result.status == pugi::status_io_error)
    {
      throw file_error(message, result.status == pugi::status_file_not_found);
    }
    throw parse_error(message);
  }

  return ReadGpxXml(doc);
}

Gpx ParseGpx(const std::string& data)
{
  // U+0000 is excluded from the XML `Char` production, so a NUL byte anywhere makes the document
  // ill-formed. pugixml does not report it: it zero-terminates its own copy of the buffer and uses
  // NUL as the parser's end sentinel, so an embedded NUL is indistinguishable from end of input
  // and everything after it is dropped. Passing the explicit length does not help. Reject it here
  // instead, so that a truncated or corrupted document fails rather than parsing as a short one.
  if (const auto nul = data.find('\0'); nul != std::string::npos)
  {
    const auto message =
        std::format("Failed to parse GPX data: NUL byte at offset {} is not valid XML", nul);
    throw parse_error(message);
  }

  pugi::xml_document doc;
  // The data is known to be NUL-free by now, so the length is redundant, but `load_buffer` avoids
  // the `strlen` that `load_string` would do. `encoding_utf8` keeps the encoding `load_string`
  // assumed.
  pugi::xml_parse_result result =
      doc.load_buffer(data.data(), data.size(), pugi::parse_default, pugi::encoding_utf8);

  if (!result)
  {
    const auto message = std::format("Failed to parse GPX data: {}", result.description());
    throw parse_error(message);
  }

  return ReadGpxXml(doc);
}

} // namespace fastgpx
