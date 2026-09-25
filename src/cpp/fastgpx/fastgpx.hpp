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
  // the object itself; longer text goes on the heap. Parsing creates one TimePoint per point, and
  // `list(segment.points)` copies every point. A heap allocation for the text would happen in both
  // places, which is why normal timestamps are kept inline.
  TimePoint(std::string_view time_string);
  TimePoint(std::chrono::system_clock::time_point time_point) noexcept;

  TimePoint(const TimePoint& other);
  TimePoint(TimePoint&& other) noexcept;
  TimePoint& operator=(const TimePoint& other);
  TimePoint& operator=(TimePoint&& other) noexcept;
  ~TimePoint();

  // Two time points are equal when they represent the same time, whether or not `value()` has
  // parsed either of them. Identical text is equal without being parsed. Text that cannot be
  // parsed is equal only to identical text. A malformed <time> never makes the comparison throw.
  // See #16.
  //
  // Times are compared at microsecond resolution, the finest that Python's `datetime` can hold,
  // and finer digits are dropped. That way a point rebuilt in Python from its own `LatLong.time`
  // compares equal to the original, on every platform.
  //
  // A date outside the range of `system_clock` counts as text that cannot be parsed. That range is
  // narrower on libstdc++ (about 1677 to 2262) than on MSVC and libc++, so the result for such
  // dates differs by platform.
  //
  // There is deliberately no ordering. Comparing the raw text would only order Zulu strings of the
  // same length correctly, and nothing in the library orders time points.
  bool operator==(const TimePoint& other) const;

  std::chrono::system_clock::time_point value() const;

  // The unparsed source text, or nullopt once `value()` has replaced it with the time point it
  // parsed to. `Segment::ComputeTimeBounds` reads it to compare timestamps without parsing them.
  // A later `value()` call on the same TimePoint releases the text, so the view must not outlive
  // it.
  std::optional<std::string_view> raw() const;

  static constexpr std::size_t kInlineCapacity = 38;

private:
  enum class Type : unsigned char
  {
    kInlineText,
    kHeapText,
    kParsed,
  };

  void StoreText(std::string_view text);
  void ReleaseHeapText() const noexcept;
  std::string_view Text() const noexcept;
  void StoreParsed(std::chrono::system_clock::time_point time_point) const noexcept;
  std::chrono::system_clock::time_point LoadParsed() const noexcept;

  // The inline text, the heap pointer and size, and the parsed time share one byte buffer.
  // `type_` says which one it holds, and `inline_size_` is the length of inline text.
  //
  // This is a byte buffer rather than a union to keep a `TimePoint` at 40 bytes, the same as the
  // `std::variant` it replaced. A union holding the 38-byte text would be padded to 40 bytes, and
  // `inline_size_` and `type_` would then make the object 48 bytes. With a byte buffer they use
  // the two bytes that would otherwise be padding. The pointer, size and time point are written
  // and read back with `std::memcpy`, which is the standard-conforming way to store an object in
  // raw bytes, with no alignment or aliasing concerns.
  alignas(8) mutable unsigned char storage_[kInlineCapacity] = {};
  mutable unsigned char inline_size_ = 0;
  mutable Type type_ = Type::kInlineText;
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
