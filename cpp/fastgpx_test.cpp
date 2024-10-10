#include "fastgpx.hpp"

#include <filesystem>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Matchers::WithinRel;

const auto project_path = std::filesystem::path(FASTGPX_PROJECT_DIR);

TEST_CASE("Two-point single segment track", "[parse][simple]")
{
  const auto path = project_path / "gpx/test/debug-segment.gpx";
  const auto gpx = fastgpx::ParseGpx(path);

  SECTION("loads all the tracks and segments")
  {
    REQUIRE(gpx.tracks.size() == 1);
    REQUIRE(gpx.tracks[0].segments.size() == 1);
  }

  SECTION("computes 2D distances")
  {
    CHECK_THAT(gpx.tracks[0].segments[0].GetLength2D(), WithinRel(1.38515855107115149));
    CHECK_THAT(gpx.tracks[0].GetLength2D(), WithinRel(1.38515855107115149));
    CHECK_THAT(gpx.GetLength2D(), WithinRel(1.38515855107115149));
  }

  SECTION("computes 3D distances")
  {
    CHECK_THAT(gpx.tracks[0].segments[0].GetLength3D(), WithinRel(1.70840984883766467));
    CHECK_THAT(gpx.tracks[0].GetLength3D(), WithinRel(1.70840984883766467));
    CHECK_THAT(gpx.GetLength3D(), WithinRel(1.70840984883766467));
  }
}

TEST_CASE("TopCamp 2024 GPX file", "[parse][real_world]")
{
  const auto path = project_path / "gpx/2024 TopCamp/Connected_20240518_094959_.gpx";
  const auto gpx = fastgpx::ParseGpx(path);

  SECTION("loads all the tracks and segments")
  {
    REQUIRE(gpx.tracks.size() == 1);
    REQUIRE(gpx.tracks[0].segments.size() == 9);
  }

  SECTION("computes 2D distances")
  {
    CHECK_THAT(gpx.tracks[0].segments[0].GetLength2D(), WithinRel(17824.19175882741183159));
    CHECK_THAT(gpx.tracks[0].GetLength2D(), WithinRel(383273.40397503407439217));
    CHECK_THAT(gpx.GetLength2D(), WithinRel(383273.40397503407439217));
  }

  SECTION("computes 3D distances")
  {
    CHECK_THAT(gpx.tracks[0].segments[0].GetLength3D(), WithinRel(17859.48257500379986595));
    CHECK_THAT(gpx.tracks[0].GetLength3D(), WithinRel(383685.94684573041740805));
    CHECK_THAT(gpx.GetLength3D(), WithinRel(383685.94684573041740805));
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
