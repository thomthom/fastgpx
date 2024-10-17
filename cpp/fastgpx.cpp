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

#include "geom.hpp"

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
