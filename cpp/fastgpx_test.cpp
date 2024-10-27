#include "fastgpx.hpp"
#include "test_data.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

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

  CHECK_THAT(gpx.tracks[0].segments[0].GetLength2D(), WithinAbs(1.3839, kMETERS_TOL));
  CHECK_THAT(gpx.tracks[0].GetLength2D(), WithinAbs(1.3839, kMETERS_TOL));
  CHECK_THAT(gpx.GetLength2D(), WithinAbs(1.3839, kMETERS_TOL));

  CHECK_THAT(gpx.tracks[0].segments[0].GetLength3D(), WithinAbs(1.7074, kMETERS_TOL));
  CHECK_THAT(gpx.tracks[0].GetLength3D(), WithinAbs(1.7074, kMETERS_TOL));
  CHECK_THAT(gpx.GetLength3D(), WithinAbs(1.7074, kMETERS_TOL));

  // TimeBounds
  // UNIX timestamp for 2024-05-18T06:50:00Z
  const auto expected_start = std::chrono::system_clock::from_time_t(1716015000);
  // UNIX timestamp for 2024-05-18T06:50:01Z
  const auto expected_end = std::chrono::system_clock::from_time_t(1716015001);

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

TEST_CASE("Parse real world GPX files", "[parse][real_world]")
{
  const auto json_path = project_path / "cpp/expected_gpx_data.json";
  const auto expected_data = LoadExpectedGpxData(json_path);

  SECTION("Matches expected values")
  {
    const auto expected = GENERATE_REF(from_range(expected_data));

    CAPTURE(expected.path);

    const auto path = project_path / expected.path;
    const auto gpx = fastgpx::ParseGpx(path);

    REQUIRE(gpx.tracks.size() == expected.tracks.size());
    for (size_t track_index = 0; track_index < gpx.tracks.size(); track_index++)
    {
      const auto &track = gpx.tracks[track_index];
      const auto &expected_track = expected.tracks[track_index];

      CAPTURE(track_index);

      REQUIRE(track.segments.size() == expected_track.segments.size());
      for (size_t segment_index = 0; segment_index < track.segments.size(); segment_index++)
      {
        const auto &segment = track.segments[segment_index];
        const auto &expected_segment = expected_track.segments[segment_index];

        CAPTURE(segment_index);

        CHECK_THAT(segment.GetLength2D(), WithinAbs(expected_segment.length2d, kMETERS_TOL));
        CHECK_THAT(segment.GetLength3D(), WithinAbs(expected_segment.length3d, kMETERS_TOL));
      }

      CHECK_THAT(track.GetLength2D(), WithinAbs(expected_track.length2d, kMETERS_TOL));
      CHECK_THAT(track.GetLength3D(), WithinAbs(expected_track.length3d, kMETERS_TOL));
    }

    CHECK_THAT(gpx.GetLength2D(), WithinAbs(expected.length2d, kMETERS_TOL));
    CHECK_THAT(gpx.GetLength3D(), WithinAbs(expected.length3d, kMETERS_TOL));
  }
}

TEST_CASE("Benchmark GPX Parsing", "[!benchmark][parse]")
{
  const auto path1 = project_path / "gpx/2024 TopCamp/Connected_20240518_094959_.gpx";
  BENCHMARK("Connected_20240518_094959_.gpx") { return fastgpx::ParseGpx(path1); };

  const auto path2 =
      project_path /
      "gpx/2024 TopCamp/Connected_20240520_103549_Lagerbergsgatan_35_45131_Uddevalla_Sweden.gpx";
  BENCHMARK("Connected_20240520_103549_Lagerbergsgatan_35_45131_Uddevalla_Sweden.gpx")
  {
    return fastgpx::ParseGpx(path2);
  };
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
    LatLong ll1{-10.0, 20.0};
    bounds.Add(ll1);
    CHECK(!bounds.IsEmpty());
    CHECK_THAT(bounds.min->latitude, WithinAbs(-10.0, 1e-8));
    CHECK_THAT(bounds.min->longitude, WithinAbs(20.0, 1e-8));
    CHECK_THAT(bounds.max->latitude, WithinAbs(-10.0, 1e-8));
    CHECK_THAT(bounds.max->longitude, WithinAbs(20.0, 1e-8));

    LatLong ll2{15.0, -5.0};
    bounds.Add(ll2);
    CHECK(!bounds.IsEmpty());
    CHECK_THAT(bounds.min->latitude, WithinAbs(-10.0, 1e-8));
    CHECK_THAT(bounds.min->longitude, WithinAbs(-5.0, 1e-8));
    CHECK_THAT(bounds.max->latitude, WithinAbs(15.0, 1e-8));
    CHECK_THAT(bounds.max->longitude, WithinAbs(20.0, 1e-8));
  }

  SECTION("Another Bounds")
  {
    Bounds other1{LatLong{-10.0, -5.0}, LatLong{15.0, 20.0}};
    bounds.Add(other1);
    CHECK(!bounds.IsEmpty());
    CHECK_THAT(bounds.min->latitude, WithinAbs(-10.0, 1e-8));
    CHECK_THAT(bounds.min->longitude, WithinAbs(-5.0, 1e-8));
    CHECK_THAT(bounds.max->latitude, WithinAbs(15.0, 1e-8));
    CHECK_THAT(bounds.max->longitude, WithinAbs(20.0, 1e-8));

    Bounds other2{LatLong{-15.0, 5.0}, LatLong{10.0, 30.0}};
    bounds.Add(other2);
    CHECK(!bounds.IsEmpty());
    CHECK_THAT(bounds.min->latitude, WithinAbs(-15.0, 1e-8));
    CHECK_THAT(bounds.min->longitude, WithinAbs(-5.0, 1e-8));
    CHECK_THAT(bounds.max->latitude, WithinAbs(15.0, 1e-8));
    CHECK_THAT(bounds.max->longitude, WithinAbs(30.0, 1e-8));
  }
}
