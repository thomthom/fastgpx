#pragma once

#include <chrono>
#include <compare>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace fastgpx {

class TimePoint
{
public:
  // Keeps a copy of the unparsed <time> text. Timestamps up to `kInlineCapacity` characters, which
  // covers every form `parse_gpx_time` accepts short of an unusually long fraction, are stored in
  // the object itself; longer text goes on the heap. Parsing stores one per point and
  // `list(segment.points)` copies every point, so an allocation here was paid on both.
  TimePoint(std::string_view time_string);
  TimePoint(std::chrono::system_clock::time_point time_point) noexcept;

  TimePoint(const TimePoint& other);
  TimePoint(TimePoint&& other) noexcept;
  TimePoint& operator=(const TimePoint& other);
  TimePoint& operator=(TimePoint&& other) noexcept;
  ~TimePoint();

  // Two time points are equal when they name the same instant, whether or not `value()` has
  // already parsed either of them. A timestamp that cannot be parsed at all is equal only to the
  // identical text, so comparing points from a file with a malformed <time> is still well defined
  // and cannot throw. See #16.
  //
  // Instants are compared at microsecond resolution, truncated towards the past. That is what
  // `datetime.datetime` holds, so a point rebuilt from its own `LatLong.time` compares equal to
  // it, and the result is the same on every platform whatever the resolution of `system_clock`.
  //
  // "Cannot be parsed" includes a date outside the range of `system_clock`, which is narrower on
  // some platforms than the four digit years the format allows, so two spellings of one instant
  // outside that range compare equal where the clock reaches them and unequal where it does not.
  //
  // There is deliberately no ordering. Comparing the raw text would order same-length Zulu
  // strings correctly and nothing else, and nothing in the library orders time points.
  bool operator==(const TimePoint& other) const;

  // A hash consistent with `operator==`: the instant truncated to microseconds, or the text when
  // it cannot be parsed. Parses the text each time without storing the result, like `==`.
  std::size_t Hash() const;

  std::chrono::system_clock::time_point value() const;

  // The unparsed source text, or nullopt once `value()` has replaced it with the time point it
  // parsed to. `Segment::ComputeTimeBounds` reads it to compare timestamps without parsing them.
  // A later `value()` call on the same TimePoint releases the text, so the view must not outlive
  // it.
  std::optional<std::string_view> raw() const;

  static constexpr std::size_t kInlineCapacity = 38;

private:
  enum class Kind : unsigned char
  {
    kInlineText,
    kHeapText,
    kParsed,
  };

  // `storage_` holds, by `kind_`: the text itself (`inline_size_` bytes), a `char*` and a
  // `std::size_t` size for heap text, or the parsed `time_point`. They are read and written with
  // `std::memcpy`, which keeps the size and the tag in what would otherwise be the tail padding of
  // a union, so a `TimePoint` is 40 bytes like the `std::variant` it replaced.
  void StoreText(std::string_view text);
  void ReleaseHeapText() const noexcept;
  std::string_view Text() const noexcept;
  void StoreParsed(std::chrono::system_clock::time_point time_point) const noexcept;
  std::chrono::system_clock::time_point LoadParsed() const noexcept;

  alignas(8) mutable unsigned char storage_[kInlineCapacity] = {};
  mutable unsigned char inline_size_ = 0;
  mutable Kind kind_ = Kind::kInlineText;
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

  // Equality only: `time` has no ordering. See `TimePoint::operator==`.
  bool operator==(const LatLong&) const = default;

  // A hash consistent with `operator==`, for Python's `__hash__`. Not computed or stored while
  // parsing; each call hashes the point afresh.
  std::size_t Hash() const;
};

struct Bounds
{
  std::optional<LatLong> min = std::nullopt;
  std::optional<LatLong> max = std::nullopt;

  bool operator==(const Bounds&) const = default;

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

Gpx LoadGpx(const std::filesystem::path& path);

Gpx ParseGpx(std::string_view data);

} // namespace fastgpx
