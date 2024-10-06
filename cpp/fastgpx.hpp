#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <pugixml.hpp>

namespace fastgpx
{

  struct LatLong
  {
    double latitude = 0.0;
    double longitude = 0.0;
    double elevation = 0.0;
  };

  // Represent <trkseg> data in GPX files.
  struct Segment
  {
    std::vector<LatLong> points;
    // <extensions>

    double GetLength2D() const;
    double GetLength3D() const;

  private:
    double ComputeLength2D() const;
    double ComputeLength3D() const;

    mutable std::optional<double> length2D;
    mutable std::optional<double> length3D;
  };

  // Represent <trk> data in GPX files.
  struct Track
  {
    std::optional<std::string> name;
    std::optional<std::string> comment;
    std::optional<std::string> description;
    // <link>
    std::optional<size_t> number;
    std::optional<std::string> type;
    // <extensions>
    std::vector<Segment> segments; // <trkseg>

    double GetLength2D() const;
    double GetLength3D() const;

  private:
    double ComputeLength2D() const;
    double ComputeLength3D() const;

    mutable std::optional<double> length2D;
    mutable std::optional<double> length3D;
  };

  struct Gpx
  {
    // <metadata>
    // <wpt>
    // <tre>
    std::vector<Track> tracks; // <trk>

    double GetLength2D() const;
    double GetLength3D() const;

  private:
    double ComputeLength2D() const;
    double ComputeLength3D() const;

    mutable std::optional<double> length2D;
    mutable std::optional<double> length3D;
  };

  Gpx ParseGpx(const std::filesystem::path &path);

} // namespace fastgpx
