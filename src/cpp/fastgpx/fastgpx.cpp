#include "fastgpx/fastgpx.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

#include "fastgpx/datetime.hpp"
#include "fastgpx/errors.hpp"
#include "fastgpx/geom.hpp"

namespace fastgpx {

// TimePoint

static_assert(sizeof(TimePoint) == 40);
static_assert(sizeof(char*) + sizeof(std::size_t) <= TimePoint::kInlineCapacity);
static_assert(sizeof(std::chrono::system_clock::time_point) <= TimePoint::kInlineCapacity);
static_assert(std::is_trivially_copyable_v<std::chrono::system_clock::time_point>);

TimePoint::TimePoint(const std::string_view time_string)
{
  StoreText(time_string);
}

TimePoint::TimePoint(const std::chrono::system_clock::time_point time_point) noexcept
{
  StoreParsed(time_point);
}

TimePoint::TimePoint(const TimePoint& other)
{
  if (other.type_ == Type::kHeapText)
  {
    StoreText(other.Text());
    return;
  }
  // Inline text and a parsed time point are plain bytes. Copying all of them, rather than only
  // the text, is a fixed-size copy of 38 bytes that the compiler turns into a few load and store
  // instructions.
  std::memcpy(storage_, other.storage_, sizeof(storage_));
  inline_size_ = other.inline_size_;
  type_ = other.type_;
}

TimePoint::TimePoint(TimePoint&& other) noexcept
{
  // Heap text changes owner along with the pointer; the moved-from object is left holding empty
  // inline text.
  std::memcpy(storage_, other.storage_, sizeof(storage_));
  inline_size_ = other.inline_size_;
  type_ = other.type_;
  other.inline_size_ = 0;
  other.type_ = Type::kInlineText;
}

TimePoint& TimePoint::operator=(const TimePoint& other)
{
  if (this != &other)
  {
    *this = TimePoint(other);
  }
  return *this;
}

TimePoint& TimePoint::operator=(TimePoint&& other) noexcept
{
  if (this != &other)
  {
    ReleaseHeapText();
    std::memcpy(storage_, other.storage_, sizeof(storage_));
    inline_size_ = other.inline_size_;
    type_ = other.type_;
    other.inline_size_ = 0;
    other.type_ = Type::kInlineText;
  }
  return *this;
}

TimePoint::~TimePoint()
{
  ReleaseHeapText();
}

void TimePoint::StoreText(const std::string_view text)
{
  if (text.size() <= kInlineCapacity)
  {
    // An empty view may have a null `data()`, and `memcpy` from a null pointer is undefined even
    // for a size of zero.
    if (!text.empty())
    {
      std::memcpy(storage_, text.data(), text.size());
    }
    inline_size_ = static_cast<unsigned char>(text.size());
    type_ = Type::kInlineText;
    return;
  }
  char* const data = new char[text.size()];
  std::memcpy(data, text.data(), text.size());
  const std::size_t size = text.size();
  std::memcpy(storage_, &data, sizeof(data));
  std::memcpy(storage_ + sizeof(data), &size, sizeof(size));
  type_ = Type::kHeapText;
}

void TimePoint::ReleaseHeapText() const noexcept
{
  if (type_ == Type::kHeapText)
  {
    char* data = nullptr;
    std::memcpy(&data, storage_, sizeof(data));
    delete[] data;
    inline_size_ = 0;
    type_ = Type::kInlineText;
  }
}

std::string_view TimePoint::Text() const noexcept
{
  assert(type_ != Type::kParsed);
  if (type_ == Type::kHeapText)
  {
    const char* data = nullptr;
    std::size_t size = 0;
    std::memcpy(&data, storage_, sizeof(data));
    std::memcpy(&size, storage_ + sizeof(data), sizeof(size));
    return {data, size};
  }
  return {reinterpret_cast<const char*>(storage_), inline_size_};
}

void TimePoint::StoreParsed(const std::chrono::system_clock::time_point time_point) const noexcept
{
  std::memcpy(storage_, &time_point, sizeof(time_point));
  type_ = Type::kParsed;
}

std::chrono::system_clock::time_point TimePoint::LoadParsed() const noexcept
{
  assert(type_ == Type::kParsed);
  std::chrono::system_clock::time_point time_point;
  std::memcpy(&time_point, storage_, sizeof(time_point));
  return time_point;
}

std::chrono::system_clock::time_point TimePoint::value() const
{
  if (type_ != Type::kParsed)
  {
    // Parse first: if it throws, the text is left as it was.
    const auto time_point = parse_gpx_time(Text());
    ReleaseHeapText();
    StoreParsed(time_point);
  }
  return LoadParsed();
}

std::optional<std::string_view> TimePoint::raw() const
{
  if (type_ == Type::kParsed)
  {
    return std::nullopt;
  }
  return Text();
}

namespace {

// The time that `time` represents, or nullopt if its text cannot be parsed. Unlike `value()` this
// neither throws nor stores the result: see `TimePoint::operator==`.
std::optional<std::chrono::sys_time<std::chrono::microseconds>> TryInstant(
    const std::optional<std::string_view>& text, const TimePoint& time)
{
  const auto time_point = text.has_value() ? try_parse_gpx_time(*text) : std::optional(time.value());
  if (!time_point.has_value())
  {
    return std::nullopt;
  }
  return std::chrono::floor<std::chrono::microseconds>(*time_point);
}

} // namespace

bool TimePoint::operator==(const TimePoint& other) const
{
  const auto text = raw();
  const auto other_text = other.raw();

  // Identical text represents the same time. This is the common case, two points read from the same
  // document that nothing has asked the time of yet, and it needs no parsing at all.
  if (text.has_value() && other_text.has_value() && *text == *other_text)
  {
    return true;
  }

  // Otherwise compare the instants. This goes through `try_parse_gpx_time` rather than `value()`
  // for two reasons: a timestamp the parser rejects must not make a comparison throw, and a
  // comparison should not write the parsed value back into the point, which would make comparing
  // the same point from two threads a data race.
  const auto time = TryInstant(text, *this);
  const auto other_time = TryInstant(other_text, other);
  return time.has_value() && other_time.has_value() && *time == *other_time;
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
  // A corner of the bounds is a coordinate (latitude, longitude, elevation), not a track point, so
  // it doesn't carry a timestamp. Copying the whole point used to put the first point's <time>
  // into `min`, which gave `min.time` an arbitrary value and made a parsed `Bounds` compare
  // unequal to a constructed one.
  const LatLong corner{location.latitude, location.longitude, location.elevation};

  // TODO: compare all values? In case min/max is not initialized correctly.
  if (min.has_value())
  {
    min->latitude = std::min(min->latitude, location.latitude);
    min->longitude = std::min(min->longitude, location.longitude);
  }
  else
  {
    min = corner;
  }

  if (max.has_value())
  {
    max->latitude = std::max(max->latitude, location.latitude);
    max->longitude = std::max(max->longitude, location.longitude);
  }
  else
  {
    max = corner;
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

namespace {

// Finds the earliest and latest timestamp of `points`. Parsing every timestamp made time bounds
// slow, about 95 ns per point (#19), so this compares the unparsed text to find the earliest and
// latest and parses only those two.
//
// Returns nullopt when the strings do not describe the order of the times they parse to: a
// timestamp that `value()` has already replaced with a time point, a form that
// `is_sortable_gpx_time` rejects, or a mix of the two Zulu lengths within one segment. The caller
// then parses every point, which is also what reports a malformed timestamp the fast path would
// otherwise skip over.
std::optional<TimeBounds> ComputeTimeBoundsFromStrings(std::span<const LatLong> points)
{
  std::optional<std::string_view> earliest;
  std::optional<std::string_view> latest;
  for (const auto& point : points)
  {
    if (!point.time.has_value())
    {
      continue;
    }
    const auto time_string = point.time->raw();
    if (!time_string.has_value() || !is_sortable_gpx_time(*time_string))
    {
      return std::nullopt;
    }
    if (!earliest.has_value())
    {
      earliest = time_string;
      latest = time_string;
      continue;
    }
    // Every string must be as long as the first one. Otherwise fall back to parsing every
    // timestamp.
    if (time_string->size() != earliest->size())
    {
      return std::nullopt;
    }
    if (*time_string < *earliest)
    {
      earliest = time_string;
    }
    else if (*time_string > *latest)
    {
      latest = time_string;
    }
  }

  TimeBounds computed_bounds;
  if (earliest.has_value())
  {
    // Every string has passed `is_sortable_gpx_time`, so it is well formed. Parsing it can only
    // fail on a date out of range: outside the range of `system_clock`, or in year 0000. If the
    // earliest and latest are in range, every time between them is too. So parsing just these two
    // catches the same errors as parsing all of them.
    computed_bounds.Add(parse_gpx_time(*earliest));
    computed_bounds.Add(parse_gpx_time(*latest));
  }
  return computed_bounds;
}

} // namespace

TimeBounds Segment::ComputeTimeBounds() const
{
  if (const auto sorted_bounds = ComputeTimeBoundsFromStrings(points); sorted_bounds.has_value())
  {
    return *sorted_bounds;
  }

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

// The whitespace XML allows around a value.
constexpr bool IsXmlWhitespace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// Parses a decimal number, accepting what `strtod` would accept except for the locale:
// surrounding whitespace and a leading '+'. Returns nullopt when the text is not a number, has
// trailing characters, or cannot be represented as a double.
//
// pugixml's `as_double()` uses `strtod`, which honors the process' LC_NUMERIC locale. A host
// application that has called `setlocale` (e.g. to "de_DE") would then parse "61.5" as 61.
// `std::from_chars` is locale independent and considerably faster.
std::optional<double> TryParseDouble(std::string_view text)
{
  // Unlike `strtod`, `std::from_chars` neither skips leading whitespace nor accepts a leading '+'.
  // The GPX schema types these values as xsd:decimal, which allows both. Real files almost never
  // have whitespace, so check one character at a time: `find_first_not_of` cost about a tenth of
  // load time on these short strings (see benchmarks/load_profile.md).
  while (!text.empty() && IsXmlWhitespace(text.front()))
  {
    text.remove_prefix(1);
  }
  while (!text.empty() && IsXmlWhitespace(text.back()))
  {
    text.remove_suffix(1);
  }
  if (text.empty())
  {
    return std::nullopt;
  }
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
  if (end != buffer.c_str() + buffer.size() || errno == ERANGE)
  {
    return std::nullopt;
  }
  return value;
#else
  double value = 0.0;
  const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || ptr != text.data() + text.size())
  {
    // Invalid input (`invalid_argument`) and out-of-range values (`result_out_of_range`) are
    // treated alike. `value` is left unmodified in either case.
    return std::nullopt;
  }
  return value;
#endif
}

// A `<trkpt>` coordinate. GPX requires `lat` and `lon` and bounds them to ±90 and ±180, so a
// point that violates that is rejected rather than silently placed at the equator (#51). The
// range check also rejects `nan` and `inf`, which `std::from_chars` accepts as numbers: the
// negated comparison is true for NaN, the same test `polyline::encode` uses.
double ParseCoordinate(const pugi::xml_node& trkpt, const char* name, double limit)
{
  const auto attribute = trkpt.attribute(name);
  if (!attribute)
  {
    throw parse_error(
        std::format("Failed to parse GPX data: <trkpt> is missing the {} attribute", name));
  }
  const std::string_view text = attribute.value();
  const auto value = TryParseDouble(text);
  if (!value)
  {
    throw parse_error(
        std::format("Failed to parse GPX data: <trkpt> {} attribute is not a valid number: \"{}\"",
                    name, text));
  }
  if (!(std::abs(*value) <= limit))
  {
    throw parse_error(std::format(
        "Failed to parse GPX data: <trkpt> {} attribute is out of range: \"{}\"", name, text));
  }
  return *value;
}

Gpx ReadGpxXml(const pugi::xml_node& doc)
{
  Gpx gpx;

  pugi::xml_node root = doc.child("gpx");
  if (!root)
  {
    // A null node returns null nodes for every child lookup, so without this check a document
    // with some other root (a TCX or KML export, an HTML error page saved by a download script)
    // would silently produce a Gpx with no tracks.
    throw parse_error("Failed to parse GPX data: missing <gpx> root element");
  }

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
        const double lat = ParseCoordinate(trkpt, "lat", 90.0);
        const double lon = ParseCoordinate(trkpt, "lon", 180.0);

        // <ele>
        /*
        Elevation (in meters) of the point.
        */
        // Missing or unparseable elevation defaults to 0.0. See #70.
        double elevation = 0.0;
        const auto ele = trkpt.child("ele");
        if (ele)
        {
          elevation = TryParseDouble(ele.text().get()).value_or(0.0);
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
          point.time.emplace(std::string_view(time.text().as_string()));
        }
      }
    }
  }

  return gpx;
}

struct FileBuffer
{
  std::unique_ptr<char[]> data;
  std::size_t size = 0;
};

// Reads the whole file into memory. pugixml's `load_file` does the same internally but never
// exposes the bytes, which is what the NUL check in `LoadGpx` needs. The buffer is uninitialised
// and sized exactly; `load_buffer_inplace` parses it without a copy, so peak memory is one file
// buffer plus the DOM, as with `load_file`.
FileBuffer ReadFile(const std::filesystem::path& path)
{
  const auto fail = [&path](const std::string& description, bool not_found) {
    throw file_error(std::format("Failed to load GPX file: {} - {}", description, path.string()),
                     not_found);
  };

  std::error_code ec;
  const auto file_size = std::filesystem::file_size(path, ec);
  if (ec)
  {
    fail(ec.message(), ec == std::errc::no_such_file_or_directory);
  }
  const auto size = static_cast<std::size_t>(file_size);

#ifdef _WIN32
  std::FILE* raw_file = nullptr;
  if (_wfopen_s(&raw_file, path.c_str(), L"rb") != 0)
  {
    raw_file = nullptr;
  }
#else
  std::FILE* raw_file = std::fopen(path.c_str(), "rb");
#endif
  if (!raw_file)
  {
    const auto error = std::error_code(errno, std::generic_category());
    fail(error.message(), error == std::errc::no_such_file_or_directory);
  }
  const std::unique_ptr<std::FILE, int (*)(std::FILE*)> file(raw_file, &std::fclose);

  FileBuffer buffer{.data = std::make_unique_for_overwrite<char[]>(size), .size = size};
  if (std::fread(buffer.data.get(), 1, buffer.size, file.get()) != buffer.size)
  {
    fail("error reading file", false);
  }
  return buffer;
}

} // namespace

Gpx LoadGpx(const std::filesystem::path& path)
{
  FileBuffer buffer = ReadFile(path);
  const std::string_view data(buffer.data.get(), buffer.size);

  // Same check as in `ParseGpx`: pugixml stops at a NUL byte without reporting it, so a corrupted
  // or partially written file would load as a truncated document. The exception is a UTF-16 or
  // UTF-32 file, where NUL bytes are part of every character. pugixml detects those from the BOM
  // or the first character, and every such signature has a NUL within the first four bytes (see
  // `guess_buffer_encoding`), so a NUL that early is left to pugixml's encoding detection. Any
  // later NUL means the file is 8-bit and the byte is not valid XML.
  if (const auto nul = data.find('\0'); nul != std::string_view::npos && nul >= 4)
  {
    throw parse_error(
        std::format("Failed to load GPX file: NUL byte at offset {} is not valid XML - {}", nul,
                    path.string()));
  }

  // `parse_default` and `encoding_auto` are what `load_file` used. The document refers into
  // `buffer` until it is destroyed, so `buffer` outlives `ReadGpxXml`.
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_buffer_inplace(buffer.data.get(), buffer.size);

  if (!result)
  {
    const auto message =
        std::format("Failed to load GPX file: {} - {}", result.description(), path.string());
    throw parse_error(message);
  }

  return ReadGpxXml(doc);
}

Gpx ParseGpx(std::string_view data)
{
  // U+0000 is excluded from the XML `Char` production, so a NUL byte anywhere makes the document
  // ill-formed. pugixml does not report it: it zero-terminates its own copy of the buffer and uses
  // NUL as the parser's end sentinel, so an embedded NUL is indistinguishable from end of input
  // and everything after it is dropped. Passing the explicit length does not help. Reject it here
  // instead, so that a truncated or corrupted document fails rather than parsing as a short one.
  if (const auto nul = data.find('\0'); nul != std::string_view::npos)
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
