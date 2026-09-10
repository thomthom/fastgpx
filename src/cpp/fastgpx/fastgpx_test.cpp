#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <ratio>
#include <sstream>
#include <string>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "fastgpx/datetime.hpp"
#include "fastgpx/errors.hpp"
#include "fastgpx/fastgpx.hpp"
#include "fastgpx/test_data.hpp"

using Catch::Generators::from_range;
using Catch::Generators::table;
using Catch::Matchers::WithinAbs;

using namespace fastgpx;

// Sufficient tolerance for comparing meters.
constexpr double kMETERS_TOL = 1e-4;

const auto project_path = std::filesystem::path(FASTGPX_PROJECT_DIR);

// Gpx

TEST_CASE("Parse two-point single segment track", "[parse][simple]")
{
  const auto path = project_path / "gpx/test/debug-segment.gpx";
  const auto gpx = fastgpx::LoadGpx(path);

  REQUIRE(gpx.tracks.size() == 1);
  REQUIRE(gpx.tracks[0].segments.size() == 1);

  CHECK(!gpx.name.has_value());

  CHECK_THAT(gpx.tracks[0].segments[0].GetLength2D(), WithinAbs(1.3839, kMETERS_TOL));
  CHECK_THAT(gpx.tracks[0].GetLength2D(), WithinAbs(1.3839, kMETERS_TOL));
  CHECK_THAT(gpx.GetLength2D(), WithinAbs(1.3839, kMETERS_TOL));

  CHECK_THAT(gpx.tracks[0].segments[0].GetLength3D(), WithinAbs(1.7074, kMETERS_TOL));
  CHECK_THAT(gpx.tracks[0].GetLength3D(), WithinAbs(1.7074, kMETERS_TOL));
  CHECK_THAT(gpx.GetLength3D(), WithinAbs(1.7074, kMETERS_TOL));

  // TimeBounds
  // https://www.timestamp-converter.com/
  // UNIX timestamp for 2024-05-18T07:50:00Z
  const auto expected_start = std::chrono::system_clock::from_time_t(1716018600);
  // UNIX timestamp for 2024-05-18T07:50:01Z
  const auto expected_end = std::chrono::system_clock::from_time_t(1716018601);

  const auto segment_time_bounds = gpx.tracks[0].segments[0].GetTimeBounds();
  REQUIRE(segment_time_bounds.start_time.has_value());
  REQUIRE(segment_time_bounds.end_time.has_value());
  CHECK(*segment_time_bounds.start_time == expected_start);
  CHECK(*segment_time_bounds.end_time == expected_end);

  const auto track_time_bounds = gpx.tracks[0].GetTimeBounds();
  REQUIRE(track_time_bounds.start_time.has_value());
  REQUIRE(track_time_bounds.end_time.has_value());
  CHECK(*track_time_bounds.start_time == expected_start);
  CHECK(*track_time_bounds.end_time == expected_end);

  const auto gpx_time_bounds = gpx.GetTimeBounds();
  REQUIRE(gpx_time_bounds.start_time.has_value());
  REQUIRE(gpx_time_bounds.end_time.has_value());
  CHECK(*gpx_time_bounds.start_time == expected_start);
  CHECK(*gpx_time_bounds.end_time == expected_end);
}

TEST_CASE("Parse time bounds of real world GPX file", "[parse][simple]")
{
  const auto path =
      project_path /
      "gpx/2024 TopCamp/Connected_20240529_091916_Harald_Bothners_Veg_36_7052_Trondheim.gpx";
  const auto gpx = fastgpx::LoadGpx(path);

  CHECK(gpx.name.value() == "Harald Bothners Veg 36, 7052 Trondheim");

  REQUIRE(gpx.tracks.size() == 1);

  // TimeBounds
  // https://www.timestamp-converter.com/
  // UNIX timestamp for 2024-05-29T07:19:16Z
  const auto expected_start = std::chrono::system_clock::from_time_t(1716967156);
  // UNIX timestamp for 2024-05-29T10:43:32Z
  const auto expected_end = std::chrono::system_clock::from_time_t(1716979412);

  const auto gpx_time_bounds = gpx.GetTimeBounds();
  REQUIRE(gpx_time_bounds.start_time.has_value());
  REQUIRE(gpx_time_bounds.end_time.has_value());
  CHECK(*gpx_time_bounds.start_time == expected_start);
  CHECK(*gpx_time_bounds.end_time == expected_end);
}

TEST_CASE("Parse bounds of real world GPX file", "[parse][simple]")
{
  const auto path =
      project_path /
      "gpx/2024 TopCamp/Connected_20240529_091916_Harald_Bothners_Veg_36_7052_Trondheim.gpx";
  const auto gpx = fastgpx::LoadGpx(path);

  CHECK(gpx.name.value() == "Harald Bothners Veg 36, 7052 Trondheim");

  REQUIRE(gpx.tracks.size() == 1);

  const fastgpx::LatLong expected_min(61.841507, 9.090457);
  const fastgpx::LatLong expected_max(63.422417, 10.44321899);

  const auto gpx_bounds = gpx.GetBounds();
  REQUIRE(!gpx_bounds.IsEmpty());
  CHECK_THAT(gpx_bounds.min->latitude, WithinAbs(expected_min.latitude, 1e-8));
  CHECK_THAT(gpx_bounds.min->longitude, WithinAbs(expected_min.longitude, 1e-8));
  CHECK_THAT(gpx_bounds.max->latitude, WithinAbs(expected_max.latitude, 1e-8));
  CHECK_THAT(gpx_bounds.max->longitude, WithinAbs(expected_max.longitude, 1e-8));
}

TEST_CASE("Parse real world GPX files", "[parse][real_world]")
{
  const auto json_path = project_path / "src/cpp/expected_gpx_data.json";
  const auto expected_data = LoadExpectedGpxData(json_path);

  SECTION("Matches expected values")
  {
    const auto expected_gpx = GENERATE_REF(from_range(expected_data));

    CAPTURE(expected_gpx.path);

    const auto path = project_path / expected_gpx.path;
    const auto gpx = fastgpx::LoadGpx(path);

    REQUIRE(gpx.tracks.size() == expected_gpx.tracks.size());
    for (size_t track_index = 0; track_index < gpx.tracks.size(); track_index++)
    {
      const auto& track = gpx.tracks[track_index];
      const auto& expected_track = expected_gpx.tracks[track_index];

      CAPTURE(track_index);

      REQUIRE(track.segments.size() == expected_track.segments.size());
      for (size_t segment_index = 0; segment_index < track.segments.size(); segment_index++)
      {
        const auto& segment = track.segments[segment_index];
        const auto& expected_segment = expected_track.segments[segment_index];

        CAPTURE(segment_index);

        CHECK_THAT(segment.GetLength2D(), WithinAbs(expected_segment.length2d, kMETERS_TOL));
        CHECK_THAT(segment.GetLength3D(), WithinAbs(expected_segment.length3d, kMETERS_TOL));

        const auto segment_time = segment.GetTimeBounds();
        REQUIRE(segment_time.IsEmpty() == expected_segment.time_bounds.IsEmpty());
        if (!segment_time.IsEmpty())
        {
          CHECK(*segment_time.start_time == *expected_segment.time_bounds.start_time);
          CHECK(*segment_time.end_time == *expected_segment.time_bounds.end_time);
        }
      }

      CHECK_THAT(track.GetLength2D(), WithinAbs(expected_track.length2d, kMETERS_TOL));
      CHECK_THAT(track.GetLength3D(), WithinAbs(expected_track.length3d, kMETERS_TOL));

      const auto track_time = track.GetTimeBounds();
      REQUIRE(track_time.IsEmpty() == expected_track.time_bounds.IsEmpty());
      if (!track_time.IsEmpty())
      {
        CHECK(*track_time.start_time == *expected_track.time_bounds.start_time);
        CHECK(*track_time.end_time == *expected_track.time_bounds.end_time);
      }
    }

    CHECK_THAT(gpx.GetLength2D(), WithinAbs(expected_gpx.length2d, kMETERS_TOL));
    CHECK_THAT(gpx.GetLength3D(), WithinAbs(expected_gpx.length3d, kMETERS_TOL));

    const auto gpx_time = gpx.GetTimeBounds();
    REQUIRE(gpx_time.IsEmpty() == expected_gpx.time_bounds.IsEmpty());
    if (!gpx_time.IsEmpty())
    {
      CHECK(*gpx_time.start_time == *expected_gpx.time_bounds.start_time);
      CHECK(*gpx_time.end_time == *expected_gpx.time_bounds.end_time);
    }
  }
}

TEST_CASE("Benchmark GPX Parsing", "[!benchmark][parse]")
{
  const auto path1 = project_path / "gpx/2024 TopCamp/Connected_20240518_094959_.gpx";
  BENCHMARK("Connected_20240518_094959_.gpx")
  {
    return fastgpx::LoadGpx(path1);
  };

  const auto path2 =
      project_path /
      "gpx/2024 TopCamp/Connected_20240520_103549_Lagerbergsgatan_35_45131_Uddevalla_Sweden.gpx";
  BENCHMARK("Connected_20240520_103549_Lagerbergsgatan_35_45131_Uddevalla_Sweden.gpx")
  {
    return fastgpx::LoadGpx(path2);
  };
}

namespace {

Segment MakeTimedSegment(const std::vector<std::string>& timestamps)
{
  Segment segment;
  for (const auto& timestamp : timestamps)
  {
    LatLong point;
    point.time = TimePoint(timestamp);
    segment.points.push_back(point);
  }
  return segment;
}

// Compares `Segment::GetTimeBounds` against the bounds of every timestamp parsed one by one,
// which is what the segment does when it cannot compare the strings.
void CheckTimeBounds(const std::vector<std::string>& timestamps)
{
  CAPTURE(timestamps);

  TimeBounds expected;
  for (const auto& timestamp : timestamps)
  {
    expected.Add(parse_gpx_time(timestamp));
  }

  const auto segment = MakeTimedSegment(timestamps);
  CHECK(segment.GetTimeBounds() == expected);
}

} // namespace

TEST_CASE("TimePoint equality", "[timepoint]")
{
  // A `TimePoint` holds either the unparsed <time> text or the instant it parses to, and the two
  // states have to compare the same. See #16.
  const auto instant = parse_gpx_time("2024-05-18T07:50:01Z");

  SECTION("identical text")
  {
    CHECK(TimePoint("2024-05-18T07:50:01Z") == TimePoint("2024-05-18T07:50:01Z"));
    CHECK_FALSE(TimePoint("2024-05-18T07:50:01Z") == TimePoint("2024-05-18T07:50:02Z"));
  }
  SECTION("text against the instant it parses to")
  {
    CHECK(TimePoint("2024-05-18T07:50:01Z") == TimePoint(instant));
    CHECK(TimePoint(instant) == TimePoint("2024-05-18T07:50:01Z"));
  }
  SECTION("equality is transitive across all three states")
  {
    const TimePoint zulu("2024-05-18T07:50:01Z");
    const TimePoint offset("2024-05-18T09:50:01+02:00");
    const TimePoint parsed(instant);
    const TimePoint unparseable("not a time");

    CHECK(zulu == offset);
    CHECK(offset == parsed);
    CHECK(zulu == parsed);

    CHECK_FALSE(unparseable == zulu);
    CHECK_FALSE(unparseable == offset);
    CHECK_FALSE(unparseable == parsed);
    // The instant on the left takes the branch that reads the stored time point rather than
    // parsing, which must not throw for a right hand side that will not parse.
    CHECK_FALSE(parsed == unparseable);
  }
  SECTION("an empty timestamp")
  {
    // `<time></time>` produces this.
    CHECK(TimePoint("") == TimePoint(""));
    CHECK_FALSE(TimePoint("") == TimePoint("2024-05-18T07:50:01Z"));
    CHECK_FALSE(TimePoint("") == TimePoint(instant));
  }
  SECTION("the same instant written differently")
  {
    CHECK(TimePoint("2024-05-18T07:50:01Z") == TimePoint("2024-05-18T07:50:01.000Z"));
    CHECK(TimePoint("2024-05-18T07:50:01Z") == TimePoint("2024-05-18T09:50:01+02:00"));
    CHECK(TimePoint("2024-05-18T07:50:01Z") == TimePoint("2024-05-18T07:50:01"));
  }
  SECTION("comparing does not parse the stored text")
  {
    // `value()` caches, comparison deliberately does not, so that comparing the same point from
    // two threads is not a data race.
    const TimePoint time_point("2024-05-18T07:50:01Z");
    CHECK(time_point == TimePoint(instant));
    CHECK(time_point.raw() != nullptr);
  }
  SECTION("a timestamp that cannot be parsed compares by its text")
  {
    CHECK(TimePoint("not a time") == TimePoint("not a time"));
    CHECK_FALSE(TimePoint("not a time") == TimePoint("also not a time"));
    CHECK_FALSE(TimePoint("not a time") == TimePoint(instant));
    CHECK_FALSE(TimePoint("not a time") == TimePoint("2024-05-18T07:50:01Z"));
  }
  SECTION("a timestamp outside the representable range compares by its text")
  {
    // Year 0 is rejected on every platform, so neither side has an instant to compare.
    CHECK(TimePoint("0000-01-01T00:00:00Z") == TimePoint("0000-01-01T00:00:00Z"));
    CHECK_FALSE(TimePoint("0000-01-01T00:00:00Z") == TimePoint("0000-01-02T00:00:00Z"));
  }
}

TEST_CASE("LatLong equality", "[timepoint]")
{
  // `LatLong` compares its time along with its coordinates, so a point read from a document has to
  // compare equal to one built with the same instant. See #16.
  LatLong parsed{60.5, 10.5, 100.0, TimePoint(std::string("2024-05-18T07:50:01Z"))};
  LatLong built{60.5, 10.5, 100.0, TimePoint(parse_gpx_time("2024-05-18T07:50:01Z"))};
  CHECK(parsed == built);

  built.time = TimePoint(parse_gpx_time("2024-05-18T07:50:02Z"));
  CHECK(parsed != built);

  built.time = std::nullopt;
  CHECK(parsed != built);
}

TEST_CASE("Segment points compare equal after their times have been parsed", "[timepoint]")
{
  // The one route that converts the *stored* points rather than a copy: a segment whose timestamps
  // `is_sortable_gpx_time` rejects falls back to reading every point's time. The points must still
  // compare equal to ones carrying the same instants. See #16.
  const auto segment = MakeTimedSegment({"2024-05-18T09:50:01+02:00", "2024-05-18T09:50:02+02:00"});
  const auto expected =
      MakeTimedSegment({"2024-05-18T09:50:01+02:00", "2024-05-18T09:50:02+02:00"});

  (void)segment.GetTimeBounds();
  REQUIRE(segment.points.front().time->raw() == nullptr);
  REQUIRE(expected.points.front().time->raw() != nullptr);

  CHECK(segment.points == expected.points);
}

TEST_CASE("Bounds equality", "[bounds]")
{
  // A corner of a bounding box carries no timestamp, so bounds computed from timed points compare
  // equal to bounds built from the coordinates alone. See #16.
  const auto segment = MakeTimedSegment({"2024-05-18T07:50:01Z", "2024-05-18T07:50:02Z"});
  Segment placed = segment;
  placed.points[0].latitude = 60.5;
  placed.points[0].longitude = 10.5;
  placed.points[1].latitude = 61.5;
  placed.points[1].longitude = 11.5;

  const auto bounds = placed.GetBounds();
  REQUIRE(bounds.min.has_value());
  CHECK_FALSE(bounds.min->time.has_value());
  CHECK_FALSE(bounds.max->time.has_value());

  const Bounds expected{LatLong{60.5, 10.5, 0.0}, LatLong{61.5, 11.5, 0.0}};
  CHECK(bounds == expected);
}

TEST_CASE("Segment time bounds", "[timebounds]")
{
  // The bounds are found by comparing the unparsed strings when every timestamp is of one
  // sortable form, and by parsing every point otherwise. Both paths must give the same answer.
  // See #19.
  SECTION("seconds only, out of order")
  {
    CheckTimeBounds({"2024-05-18T07:50:03Z", "2024-05-18T07:50:01Z", "2024-05-18T07:50:02Z"});
  }
  SECTION("the string comparison leaves the points it did not need alone")
  {
    // Nothing in the bounds themselves says which path ran, so this pins the observable
    // difference: the fast path never asks a point for its parsed time, so every point still
    // holds the string it was parsed from.
    const auto segment =
        MakeTimedSegment({"2024-05-18T07:50:01Z", "2024-05-18T07:50:02Z", "2024-05-18T07:50:03Z"});
    (void)segment.GetTimeBounds();
    for (const auto& point : segment.points)
    {
      CHECK(point.time->raw() != nullptr);
    }
  }
  SECTION("the fallback parses every point")
  {
    // A timezone offset is not sortable, so this goes through `TimePoint::value()`, which
    // replaces each string with the time point it parsed to.
    const auto segment = MakeTimedSegment({"2024-05-18T09:50:01+02:00", "2024-05-18T07:50:02Z"});
    (void)segment.GetTimeBounds();
    for (const auto& point : segment.points)
    {
      CHECK(point.time->raw() == nullptr);
    }
  }
  SECTION("milliseconds, out of order")
  {
    CheckTimeBounds(
        {"2024-05-18T07:50:01.500Z", "2024-05-18T07:50:01.001Z", "2024-05-18T07:50:01.999Z"});
  }
  SECTION("a single timestamp")
  {
    CheckTimeBounds({"2024-05-18T07:50:01Z"});
  }
  SECTION("across a day boundary")
  {
    CheckTimeBounds({"2024-05-19T00:00:00Z", "2024-05-18T23:59:59Z", "2024-05-19T00:00:01Z"});
  }
  SECTION("mixed lengths")
  {
    // The 20 character string compares its 'Z' against the fraction separator of the others, so
    // this has to fall back to parsing every point.
    CheckTimeBounds({"2024-05-18T07:50:01.500Z", "2024-05-18T07:50:01Z", "2024-05-18T07:50:02Z"});
  }
  SECTION("mixed fraction separators")
  {
    CheckTimeBounds({"2024-05-18T07:50:01,900Z", "2024-05-18T07:50:01.100Z"});
  }
  SECTION("hour 24 carries into the next day")
  {
    // 24:30 is 00:30 the next day, which sorts before 00:15 as a string but after it in time.
    CheckTimeBounds({"2024-05-18T24:30:00Z", "2024-05-19T00:15:00Z"});
  }
  SECTION("second 60 carries into the next minute")
  {
    CheckTimeBounds({"2024-05-18T07:50:60.500Z", "2024-05-18T07:51:00.400Z"});
  }
  SECTION("a day past the end of the month carries into the next one")
  {
    CheckTimeBounds({"2024-02-31T10:00:00Z", "2024-03-01T12:00:00Z"});
  }
  SECTION("timezone offsets")
  {
    CheckTimeBounds({"2024-05-18T09:50:01+02:00", "2024-05-18T07:50:02Z"});
  }
  SECTION("no timezone designator")
  {
    CheckTimeBounds({"2024-05-18T07:50:03", "2024-05-18T07:50:01"});
  }
  SECTION("points without a time are skipped")
  {
    Segment segment = MakeTimedSegment({"2024-05-18T07:50:03Z", "2024-05-18T07:50:01Z"});
    segment.points.insert(segment.points.begin(), LatLong{});
    segment.points.push_back(LatLong{});

    const auto bounds = segment.GetTimeBounds();
    REQUIRE(bounds.IsRange());
    CHECK(*bounds.start_time == parse_gpx_time("2024-05-18T07:50:01Z"));
    CHECK(*bounds.end_time == parse_gpx_time("2024-05-18T07:50:03Z"));
  }
  SECTION("a segment without any time is empty")
  {
    Segment segment;
    segment.points.push_back(LatLong{});
    CHECK(segment.GetTimeBounds().IsEmpty());
  }
  SECTION("an empty segment is empty")
  {
    const Segment segment;
    CHECK(segment.GetTimeBounds().IsEmpty());
  }
  SECTION("timestamps that were already parsed")
  {
    // Reading `point.time->value()` replaces the string with the time point it parsed to, and
    // there is then nothing left to compare.
    const auto segment = MakeTimedSegment({"2024-05-18T07:50:03Z", "2024-05-18T07:50:01Z"});
    (void)segment.points.front().time->value();

    const auto bounds = segment.GetTimeBounds();
    REQUIRE(bounds.IsRange());
    CHECK(*bounds.start_time == parse_gpx_time("2024-05-18T07:50:01Z"));
    CHECK(*bounds.end_time == parse_gpx_time("2024-05-18T07:50:03Z"));
  }
  SECTION("a malformed timestamp is still reported")
  {
    const auto segment =
        MakeTimedSegment({"2024-05-18T07:50:01Z", "not a time", "2024-05-18T07:50:03Z"});
    CHECK_THROWS_AS(segment.GetTimeBounds(), fastgpx::parse_error);
  }
  SECTION("a timestamp with an out of range field is still reported")
  {
    const auto segment =
        MakeTimedSegment({"2024-05-18T07:50:01Z", "2024-13-01T07:50:02Z", "2024-05-18T07:50:03Z"});
    CHECK_THROWS_AS(segment.GetTimeBounds(), fastgpx::parse_error);
  }
  SECTION("a year outside a four digit year is still reported")
  {
    const auto segment =
        MakeTimedSegment({"2024-05-18T07:50:01Z", "0000-01-01T00:00:00Z", "2024-05-18T07:50:03Z"});
    CHECK_THROWS_AS(segment.GetTimeBounds(), fastgpx::parse_error);
  }
  SECTION("a timestamp the platform cannot represent is still reported")
  {
    // The fast path only parses the earliest and the latest timestamp, which is why it can decide
    // for the ones in between: anything between two representable times is representable itself.
    // Here the earliest is the one out of range.
    const auto segment = MakeTimedSegment({"2024-05-18T07:50:01Z", "1000-01-01T00:00:00Z"});
    if constexpr (std::ratio_less_equal_v<std::chrono::system_clock::period, std::nano>)
    {
      CHECK_THROWS_AS(segment.GetTimeBounds(), fastgpx::parse_error);
    }
    else
    {
      const auto bounds = segment.GetTimeBounds();
      REQUIRE(bounds.IsRange());
      CHECK(*bounds.start_time == parse_gpx_time("1000-01-01T00:00:00Z"));
    }
  }
}

TEST_CASE("Benchmark time bounds", "[!benchmark][timebounds]")
{
  // `GetTimeBounds` caches its result, so every run needs a segment whose points have not been
  // visited yet.
  const auto path = project_path / "gpx/2024 TopCamp/Connected_20240518_094959_.gpx";
  const auto gpx = fastgpx::LoadGpx(path);

  const Segment* largest = nullptr;
  for (const auto& track : gpx.tracks)
  {
    for (const auto& segment : track.segments)
    {
      if (largest == nullptr || segment.points.size() > largest->points.size())
      {
        largest = &segment;
      }
    }
  }
  REQUIRE(largest != nullptr);
  WARN("Segment points: " << largest->points.size());

  BENCHMARK_ADVANCED("Segment::GetTimeBounds")(Catch::Benchmark::Chronometer meter)
  {
    std::vector<Segment> segments(static_cast<size_t>(meter.runs()), *largest);
    meter.measure([&segments](int i) { return segments[static_cast<size_t>(i)].GetTimeBounds(); });
  };
}

TEST_CASE("Parse string file path", "[parse][simple]")
{
  const auto path = project_path / "gpx/not-a-real-path/fake.gpx";
  REQUIRE_THROWS_AS(fastgpx::LoadGpx(path), fastgpx::file_error);
}

TEST_CASE("Parse non-existing file path", "[parse][simple]")
{
  const auto path = project_path / "gpx/test/debug-segment.gpx";
  const auto path_string = path.string();
  const auto gpx = fastgpx::LoadGpx(path_string);

  REQUIRE(gpx.tracks.size() == 1);
  REQUIRE(gpx.tracks[0].segments.size() == 1);
}

TEST_CASE("Parse GPX from XML string", "[parse][simple]")
{
  const auto path = project_path / "gpx/test/debug-segment.gpx";

  std::ifstream file(path);
  if (!file)
  {
    throw std::runtime_error("Cannot open file: " + path.string());
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  const auto data = buffer.str();

  const auto gpx = fastgpx::ParseGpx(data);

  REQUIRE(gpx.tracks.size() == 1);
  REQUIRE(gpx.tracks[0].segments.size() == 1);

  CHECK(!gpx.name.has_value());

  CHECK_THAT(gpx.tracks[0].segments[0].GetLength2D(), WithinAbs(1.3839, kMETERS_TOL));
  CHECK_THAT(gpx.tracks[0].GetLength2D(), WithinAbs(1.3839, kMETERS_TOL));
  CHECK_THAT(gpx.GetLength2D(), WithinAbs(1.3839, kMETERS_TOL));

  CHECK_THAT(gpx.tracks[0].segments[0].GetLength3D(), WithinAbs(1.7074, kMETERS_TOL));
  CHECK_THAT(gpx.tracks[0].GetLength3D(), WithinAbs(1.7074, kMETERS_TOL));
  CHECK_THAT(gpx.GetLength3D(), WithinAbs(1.7074, kMETERS_TOL));
}

TEST_CASE("Parse GPX with an embedded NUL byte", "[parse][simple]")
{
  // U+0000 is not a valid XML character. pugixml stops at it silently, so the parser rejects it
  // rather than returning the truncated prefix as a successful parse.
  const std::string head = "<gpx><trk><trkseg><trkpt lat=\"60.0\" lon=\"10.0\"/>";
  const std::string tail = "<trkpt lat=\"60.1\" lon=\"10.0\"/></trkseg></trk></gpx>";

  SECTION("NUL splitting an otherwise well-formed document")
  {
    std::string data = head;
    data += '\0';
    data += tail;
    REQUIRE_THROWS_AS(fastgpx::ParseGpx(data), fastgpx::parse_error);
  }

  SECTION("NUL after a complete document, which pugixml would silently ignore")
  {
    std::string data = head + tail;
    data += '\0';
    data += head + tail;
    REQUIRE_THROWS_AS(fastgpx::ParseGpx(data), fastgpx::parse_error);
  }

  SECTION("trailing NUL")
  {
    std::string data = head + tail;
    data += '\0';
    REQUIRE_THROWS_AS(fastgpx::ParseGpx(data), fastgpx::parse_error);
  }

  SECTION("the same document without a NUL parses")
  {
    const auto gpx = fastgpx::ParseGpx(head + tail);
    REQUIRE(gpx.tracks.size() == 1);
    REQUIRE(gpx.tracks[0].segments.size() == 1);
    CHECK(gpx.tracks[0].segments[0].points.size() == 2);
  }
}

namespace {

// A file in a fresh temporary directory, removed with the directory when the test ends.
class TempFile
{
public:
  explicit TempFile(const std::string& bytes)
      : dir_(std::filesystem::temp_directory_path() /
             std::format("fastgpx-test-{}",
                         std::chrono::steady_clock::now().time_since_epoch().count()))
  {
    std::filesystem::create_directories(dir_);
    path_ = dir_ / "test.gpx";
    std::ofstream file(path_, std::ios::binary);
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  }

  ~TempFile() { std::filesystem::remove_all(dir_); }

  const std::filesystem::path& path() const { return path_; }

private:
  std::filesystem::path dir_;
  std::filesystem::path path_;
};

// Encodes ASCII text as UTF-16 with the given byte order, preceded by the matching BOM.
std::string ToUtf16(const std::string& ascii, bool little_endian)
{
  std::string out = little_endian ? "\xff\xfe" : "\xfe\xff";
  out.reserve(2 + ascii.size() * 2);
  for (const char ch : ascii)
  {
    if (little_endian)
    {
      out += ch;
      out += '\0';
    }
    else
    {
      out += '\0';
      out += ch;
    }
  }
  return out;
}

} // namespace

TEST_CASE("Load GPX file with an embedded NUL byte", "[parse][simple]")
{
  // Same check as ParseGpx: a NUL byte after a well-formed prefix used to load as the prefix
  // alone, with no error. The exception is UTF-16 input, where NUL bytes are part of every
  // character and pugixml converts the document.
  const std::string doc =
      "<gpx><trk><trkseg><trkpt lat=\"60.0\" lon=\"10.0\"/></trkseg></trk></gpx>";

  SECTION("NUL after a complete document")
  {
    const TempFile file(doc + '\0' + doc);
    REQUIRE_THROWS_AS(fastgpx::LoadGpx(file.path()), fastgpx::parse_error);
  }

  SECTION("NUL splitting an otherwise well-formed document")
  {
    std::string data = doc;
    data.insert(doc.find("</trkseg>"), 1, '\0');
    const TempFile file(data);
    REQUIRE_THROWS_AS(fastgpx::LoadGpx(file.path()), fastgpx::parse_error);
  }

  SECTION("the same document without a NUL loads")
  {
    const TempFile file(doc);
    const auto gpx = fastgpx::LoadGpx(file.path());
    REQUIRE(gpx.tracks.size() == 1);
    CHECK(gpx.tracks[0].segments[0].points.size() == 1);
  }

  SECTION("UTF-16 LE with BOM loads")
  {
    const TempFile file(ToUtf16(doc, true));
    const auto gpx = fastgpx::LoadGpx(file.path());
    REQUIRE(gpx.tracks.size() == 1);
    CHECK(gpx.tracks[0].segments[0].points.size() == 1);
  }

  SECTION("UTF-16 BE with BOM loads")
  {
    const TempFile file(ToUtf16(doc, false));
    const auto gpx = fastgpx::LoadGpx(file.path());
    REQUIRE(gpx.tracks.size() == 1);
    CHECK(gpx.tracks[0].segments[0].points.size() == 1);
  }
}

TEST_CASE("Load empty and directory paths", "[parse][simple]")
{
  SECTION("empty file is a parse error, not a file error")
  {
    const TempFile file("");
    REQUIRE_THROWS_AS(fastgpx::LoadGpx(file.path()), fastgpx::parse_error);
  }

  SECTION("directory is a file error that is not a missing file")
  {
    const TempFile file("");
    try
    {
      fastgpx::LoadGpx(file.path().parent_path());
      FAIL("expected file_error");
    }
    catch (const fastgpx::file_error& error)
    {
      CHECK(!error.not_found());
    }
  }
}

TEST_CASE("Parse <trkpt> coordinates", "[parse][simple]")
{
  const auto parse_point = [](const std::string& trkpt) {
    const auto gpx = fastgpx::ParseGpx("<gpx><trk><trkseg>" + trkpt + "</trkseg></trk></gpx>");
    REQUIRE(gpx.tracks[0].segments[0].points.size() == 1);
    return gpx.tracks[0].segments[0].points[0];
  };

  SECTION("surrounding whitespace and a leading '+' are accepted")
  {
    const auto point = parse_point("<trkpt lat=\" +12.5 \" lon=\"\t-0.0\n\"/>");
    CHECK(point.latitude == 12.5);
    CHECK(point.longitude == 0.0);
  }

  SECTION("the range limits are inclusive")
  {
    const auto point = parse_point("<trkpt lat=\"-90\" lon=\"180\"/>");
    CHECK(point.latitude == -90.0);
    CHECK(point.longitude == 180.0);
  }

  SECTION("missing attribute")
  {
    REQUIRE_THROWS_AS(parse_point("<trkpt lon=\"10.0\"/>"), fastgpx::parse_error);
    REQUIRE_THROWS_AS(parse_point("<trkpt lat=\"60.0\"/>"), fastgpx::parse_error);
    REQUIRE_THROWS_AS(parse_point("<trkpt/>"), fastgpx::parse_error);
  }

  SECTION("not a number")
  {
    const auto text = GENERATE("", " ", "abc", "60.0abc", "0x10", "1e999", "6,5");
    CAPTURE(text);
    REQUIRE_THROWS_AS(parse_point(std::format("<trkpt lat=\"{}\" lon=\"10.0\"/>", text)),
                      fastgpx::parse_error);
    REQUIRE_THROWS_AS(parse_point(std::format("<trkpt lat=\"60.0\" lon=\"{}\"/>", text)),
                      fastgpx::parse_error);
  }

  SECTION("out of range")
  {
    REQUIRE_THROWS_AS(parse_point("<trkpt lat=\"90.5\" lon=\"10.0\"/>"), fastgpx::parse_error);
    REQUIRE_THROWS_AS(parse_point("<trkpt lat=\"-90.5\" lon=\"10.0\"/>"), fastgpx::parse_error);
    REQUIRE_THROWS_AS(parse_point("<trkpt lat=\"60.0\" lon=\"180.5\"/>"), fastgpx::parse_error);
    REQUIRE_THROWS_AS(parse_point("<trkpt lat=\"60.0\" lon=\"-180.5\"/>"), fastgpx::parse_error);
  }

  SECTION("nan and inf parse as numbers but fail the range check")
  {
    const auto text = GENERATE("nan", "NaN", "inf", "-inf", "infinity");
    CAPTURE(text);
    REQUIRE_THROWS_AS(parse_point(std::format("<trkpt lat=\"{}\" lon=\"10.0\"/>", text)),
                      fastgpx::parse_error);
    REQUIRE_THROWS_AS(parse_point(std::format("<trkpt lat=\"60.0\" lon=\"{}\"/>", text)),
                      fastgpx::parse_error);
  }

  SECTION("an unparseable <ele> is still 0.0 (#70)")
  {
    const auto point = parse_point("<trkpt lat=\"60.0\" lon=\"10.0\"><ele>abc</ele></trkpt>");
    CHECK(point.elevation == 0.0);
  }
}

TEST_CASE("Parse XML without a <gpx> root element", "[parse][simple]")
{
  // Well-formed XML that is not GPX used to parse as a Gpx with no tracks and no error.
  SECTION("other document type")
  {
    REQUIRE_THROWS_AS(fastgpx::ParseGpx("<html><body/></html>"), fastgpx::parse_error);
  }

  SECTION("<gpx> nested below another root")
  {
    REQUIRE_THROWS_AS(fastgpx::ParseGpx("<root><gpx/></root>"), fastgpx::parse_error);
  }

  SECTION("empty <gpx> root is still a valid, empty document")
  {
    const auto gpx = fastgpx::ParseGpx("<gpx/>");
    CHECK(gpx.tracks.empty());
  }
}

// Bounds

TEST_CASE("Add to Bounds", "[bounds]")
{
  Bounds bounds;
  REQUIRE(bounds.IsEmpty());
  REQUIRE_FALSE(bounds.min.has_value());
  REQUIRE_FALSE(bounds.max.has_value());

  SECTION("Single LatLong")
  {
    const LatLong ll1{-10.0, 20.0};
    bounds.Add(ll1);
    CHECK(!bounds.IsEmpty());
    CHECK_THAT(bounds.min->latitude, WithinAbs(-10.0, 1e-8));
    CHECK_THAT(bounds.min->longitude, WithinAbs(20.0, 1e-8));
    CHECK_THAT(bounds.max->latitude, WithinAbs(-10.0, 1e-8));
    CHECK_THAT(bounds.max->longitude, WithinAbs(20.0, 1e-8));

    const LatLong ll2{15.0, -5.0};
    bounds.Add(ll2);
    CHECK(!bounds.IsEmpty());
    CHECK_THAT(bounds.min->latitude, WithinAbs(-10.0, 1e-8));
    CHECK_THAT(bounds.min->longitude, WithinAbs(-5.0, 1e-8));
    CHECK_THAT(bounds.max->latitude, WithinAbs(15.0, 1e-8));
    CHECK_THAT(bounds.max->longitude, WithinAbs(20.0, 1e-8));
  }

  SECTION("Another Bounds")
  {
    const Bounds other1{LatLong{-10.0, -5.0}, LatLong{15.0, 20.0}};
    bounds.Add(other1);
    CHECK(!bounds.IsEmpty());
    CHECK_THAT(bounds.min->latitude, WithinAbs(-10.0, 1e-8));
    CHECK_THAT(bounds.min->longitude, WithinAbs(-5.0, 1e-8));
    CHECK_THAT(bounds.max->latitude, WithinAbs(15.0, 1e-8));
    CHECK_THAT(bounds.max->longitude, WithinAbs(20.0, 1e-8));

    const Bounds other2{LatLong{-15.0, 5.0}, LatLong{10.0, 30.0}};
    bounds.Add(other2);
    CHECK(!bounds.IsEmpty());
    CHECK_THAT(bounds.min->latitude, WithinAbs(-15.0, 1e-8));
    CHECK_THAT(bounds.min->longitude, WithinAbs(-5.0, 1e-8));
    CHECK_THAT(bounds.max->latitude, WithinAbs(15.0, 1e-8));
    CHECK_THAT(bounds.max->longitude, WithinAbs(30.0, 1e-8));
  }
}

TEST_CASE("Max Bounds", "[bounds]")
{
  const Bounds bounds;
  REQUIRE(bounds.IsEmpty());
  REQUIRE_FALSE(bounds.min.has_value());
  REQUIRE_FALSE(bounds.max.has_value());

  const Bounds other1{LatLong{-10.0, -5.0}, LatLong{15.0, 20.0}};
  const auto result1 = bounds.MaxBounds(other1);
  CHECK(!result1.IsEmpty());
  CHECK_THAT(result1.min->latitude, WithinAbs(-10.0, 1e-8));
  CHECK_THAT(result1.min->longitude, WithinAbs(-5.0, 1e-8));
  CHECK_THAT(result1.max->latitude, WithinAbs(15.0, 1e-8));
  CHECK_THAT(result1.max->longitude, WithinAbs(20.0, 1e-8));

  const Bounds other2{LatLong{-15.0, 5.0}, LatLong{10.0, 30.0}};
  const auto result2 = result1.MaxBounds(other2);
  CHECK(!result2.IsEmpty());
  CHECK_THAT(result2.min->latitude, WithinAbs(-15.0, 1e-8));
  CHECK_THAT(result2.min->longitude, WithinAbs(-5.0, 1e-8));
  CHECK_THAT(result2.max->latitude, WithinAbs(15.0, 1e-8));
  CHECK_THAT(result2.max->longitude, WithinAbs(30.0, 1e-8));
}
