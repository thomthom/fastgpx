#include <filesystem>
#include <string>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

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
  const auto gpx = fastgpx::ParseGpx(path);

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
  const auto gpx = fastgpx::ParseGpx(path);

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
  const auto gpx = fastgpx::ParseGpx(path);

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
    const auto gpx = fastgpx::ParseGpx(path);

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
    return fastgpx::ParseGpx(path1);
  };

  const auto path2 =
      project_path /
      "gpx/2024 TopCamp/Connected_20240520_103549_Lagerbergsgatan_35_45131_Uddevalla_Sweden.gpx";
  BENCHMARK("Connected_20240520_103549_Lagerbergsgatan_35_45131_Uddevalla_Sweden.gpx")
  {
    return fastgpx::ParseGpx(path2);
  };
}

TEST_CASE("Parse string file path", "[parse][simple]")
{
  const auto path = project_path / "gpx/not-a-real-path/fake.gpx";
  REQUIRE_THROWS_AS(fastgpx::ParseGpx(path), fastgpx::parse_error);
}

TEST_CASE("Parse non-existing file path", "[parse][simple]")
{
  const auto path = project_path / "gpx/test/debug-segment.gpx";
  const auto path_string = path.string();
  const auto gpx = fastgpx::ParseGpx(path_string);

  REQUIRE(gpx.tracks.size() == 1);
  REQUIRE(gpx.tracks[0].segments.size() == 1);
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
