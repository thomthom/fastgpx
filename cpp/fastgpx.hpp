#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace fastgpx
{

  struct LatLong
  {
    double latitude = 0.0;
    double longitude = 0.0;
    double elevation = 0.0;
  };

  struct Bounds
  {
    std::optional<LatLong> min;
    std::optional<LatLong> max;

    bool IsEmpty() const;

    void Add(const LatLong &location);
    void Add(std::span<const LatLong> locations);
    void Add(const Bounds &bounds);

    Bounds MaxBounds(const Bounds &bounds) const;
  };

  // Represent <trkseg> data in GPX files.
  struct Segment
  {
    std::vector<LatLong> points;
    // <extensions>

    const Bounds &GetBounds() const;
    double GetLength2D() const;
    double GetLength3D() const;

  private:
    Bounds ComputeBounds() const;
    double ComputeLength2D() const;
    double ComputeLength3D() const;

    mutable std::optional<Bounds> bounds;
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

    const Bounds &GetBounds() const;
    double GetLength2D() const;
    double GetLength3D() const;

  private:
    Bounds ComputeBounds() const;
    double ComputeLength2D() const;
    double ComputeLength3D() const;

    mutable std::optional<Bounds> bounds;
    mutable std::optional<double> length2D;
    mutable std::optional<double> length3D;
  };

  struct Gpx
  {
    // <metadata>
    // <wpt>
    // <tre>
    std::vector<Track> tracks; // <trk>

    const Bounds &GetBounds() const;
    double GetLength2D() const;
    double GetLength3D() const;

  private:
    Bounds ComputeBounds() const;
    double ComputeLength2D() const;
    double ComputeLength3D() const;

    mutable std::optional<Bounds> bounds;
    mutable std::optional<double> length2D;
    mutable std::optional<double> length3D;
  };

  Gpx ParseGpx(const std::filesystem::path &path);
  // pybind11 appear to mangle the unicode string when binding directly to
  // std::filesystem::path. But going via a std::string works.
  Gpx ParseGpx(const std::string &path);

} // namespace fastgpx
