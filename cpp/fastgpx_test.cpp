#include "fastgpx.hpp"

#include <filesystem>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinRel;

TEST_CASE("GPX files are parsed", "[fastgpx]")
{
  SECTION("two-point single segment track")
  {
    // TODO: Find a portable way to resolve the path to the test files.
    const auto path = std::filesystem::path("../../../gpx/test/debug-segment.gpx");
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

  SECTION("real world GPX file")
  {
    // TODO: Find a portable way to resolve the path to the test files.
    const auto path = std::filesystem::path("../../../gpx/2024 TopCamp/Connected_20240518_094959_.gpx");
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
}
