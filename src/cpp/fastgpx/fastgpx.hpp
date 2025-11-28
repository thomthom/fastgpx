#pragma once

#include <chrono>
#include <compare>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace fastgpx {

class TimePoint
{
public:
  TimePoint(const std::string& time_string) : data_(time_string) {}
  TimePoint(const std::chrono::system_clock::time_point time_point) : data_(time_point) {}

  auto operator<=>(const TimePoint&) const = default;

  std::chrono::system_clock::time_point value() const;

private:
  mutable std::variant<std::string, std::chrono::system_clock::time_point> data_;
};

struct TimeBounds
{
  std::optional<std::chrono::system_clock::time_point> start_time = std::nullopt;
  std::optional<std::chrono::system_clock::time_point> end_time = std::nullopt;

  auto operator<=>(const TimeBounds&) const = default;

  bool IsEmpty() const;
  bool IsRange() const;

  void Add(std::chrono::system_clock::time_point time_point);
  void Add(const TimeBounds& time_bounds);
};

// Represent <trkpt> data in GPX files.
struct LatLong
{
  double latitude = 0.0;
  double longitude = 0.0;
  double elevation = 0.0;
  std::optional<TimePoint> time = std::nullopt;

  auto operator<=>(const LatLong&) const = default;
};

struct Bounds
{
  std::optional<LatLong> min = std::nullopt;
  std::optional<LatLong> max = std::nullopt;

  auto operator<=>(const Bounds&) const = default;

  bool IsEmpty() const;

  void Add(const LatLong& location);
  void Add(std::span<const LatLong> locations);
  void Add(const Bounds& bounds);

  Bounds MaxBounds(const Bounds& bounds) const;
};

// Represent <trkseg> data in GPX files.
struct Segment
{
  std::vector<LatLong> points;
  // <extensions>

  const Bounds& GetBounds() const;
  double GetLength2D() const;
  double GetLength3D() const;
  const TimeBounds& GetTimeBounds() const;

private:
  Bounds ComputeBounds() const;
  double ComputeLength2D() const;
  double ComputeLength3D() const;
  TimeBounds ComputeTimeBounds() const;

  mutable std::optional<Bounds> bounds;
  mutable std::optional<double> length2D;
  mutable std::optional<double> length3D;
  mutable std::optional<TimeBounds> time_bounds;
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

  const Bounds& GetBounds() const;
  double GetLength2D() const;
  double GetLength3D() const;
  const TimeBounds& GetTimeBounds() const;

private:
  Bounds ComputeBounds() const;
  double ComputeLength2D() const;
  double ComputeLength3D() const;
  TimeBounds ComputeTimeBounds() const;

  mutable std::optional<Bounds> bounds;
  mutable std::optional<double> length2D;
  mutable std::optional<double> length3D;
  mutable std::optional<TimeBounds> time_bounds;
};

struct Gpx
{
  // <metadata>
  std::optional<std::string> name; // <name>

  // <wpt>
  // <tre>
  std::vector<Track> tracks; // <trk>

  const Bounds& GetBounds() const;
  double GetLength2D() const;
  double GetLength3D() const;
  const TimeBounds& GetTimeBounds() const;

private:
  Bounds ComputeBounds() const;
  double ComputeLength2D() const;
  double ComputeLength3D() const;
  TimeBounds ComputeTimeBounds() const;

  mutable std::optional<Bounds> bounds;
  mutable std::optional<double> length2D;
  mutable std::optional<double> length3D;
  mutable std::optional<TimeBounds> time_bounds;
};

Gpx ParseGpx(const std::filesystem::path& path);
// pybind11 appear to mangle the unicode string when binding directly to
// std::filesystem::path. But going via a std::string works.
Gpx ParseGpx(const std::string& path);

} // namespace fastgpx
