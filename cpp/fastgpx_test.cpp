#include "fastgpx.hpp"
#include "test_data.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Generators::from_range;
using Catch::Generators::table;
using Catch::Matchers::WithinRel;

using namespace fastgpx;

// Sufficient tolerance for comparing meters.
constexpr double kMETERS_TOL = 1e-4;

const auto project_path = std::filesystem::path(FASTGPX_PROJECT_DIR);

TEST_CASE("Parse two-point single segment track", "[parse][simple]")
{
  const auto path = project_path / "gpx/test/debug-segment.gpx";
  const auto gpx = fastgpx::ParseGpx(path);

  REQUIRE(gpx.tracks.size() == 1);
  REQUIRE(gpx.tracks[0].segments.size() == 1);

  CHECK_THAT(gpx.tracks[0].segments[0].GetLength2D(), WithinRel(1.3851, kMETERS_TOL));
  CHECK_THAT(gpx.tracks[0].GetLength2D(), WithinRel(1.3851, kMETERS_TOL));
  CHECK_THAT(gpx.GetLength2D(), WithinRel(1.3851, kMETERS_TOL));

  CHECK_THAT(gpx.tracks[0].segments[0].GetLength3D(), WithinRel(1.7084, kMETERS_TOL));
  CHECK_THAT(gpx.tracks[0].GetLength3D(), WithinRel(1.7084, kMETERS_TOL));
  CHECK_THAT(gpx.GetLength3D(), WithinRel(1.7084, kMETERS_TOL));
}

TEST_CASE("Parse real world GPX files", "[parse][real_world]")
{
  const auto json_path = project_path / "cpp/expected_gpx_data.json";
  const auto expected_data = LoadExpectedGpxData(json_path);

  SECTION("Matches expected values")
  {
    const auto [gpx_path_name, expected] = GENERATE_REF(from_range(expected_data));

    const auto path = project_path / gpx_path_name;
    const auto gpx = fastgpx::ParseGpx(path);

    CAPTURE(gpx_path_name);

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

        CHECK_THAT(segment.GetLength2D(), WithinRel(expected_segment.length2d, kMETERS_TOL));
        CHECK_THAT(segment.GetLength3D(), WithinRel(expected_segment.length3d, kMETERS_TOL));
      }

      CHECK_THAT(track.GetLength2D(), WithinRel(expected_track.length2d, kMETERS_TOL));
      CHECK_THAT(track.GetLength3D(), WithinRel(expected_track.length3d, kMETERS_TOL));
    }

    CHECK_THAT(gpx.GetLength2D(), WithinRel(expected.length2d, kMETERS_TOL));
    CHECK_THAT(gpx.GetLength3D(), WithinRel(expected.length3d, kMETERS_TOL));
  }
}

TEST_CASE("Benchmark GPX Parsing", "[!benchmark][parse]")
{
  const auto path1 = project_path / "gpx/2024 TopCamp/Connected_20240518_094959_.gpx";
  BENCHMARK("Connected_20240518_094959_.gpx")
  {
    return fastgpx::ParseGpx(path1);
  };

  const auto path2 = project_path / "gpx/2024 TopCamp/Connected_20240520_103549_Lagerbergsgatan_35_45131_Uddevalla_Sweden.gpx";
  BENCHMARK("Connected_20240520_103549_Lagerbergsgatan_35_45131_Uddevalla_Sweden.gpx")
  {
    return fastgpx::ParseGpx(path2);
  };
}
