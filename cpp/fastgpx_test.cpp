#include "fastgpx.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <filesystem>

using Catch::Matchers::WithinRel;

TEST_CASE("GPX files are parsed", "[fastgpx]")
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
    REQUIRE_THAT(gpx.tracks[0].segments[0].GetLength2D(), WithinRel(1.38515855107115149));
    REQUIRE_THAT(gpx.tracks[0].GetLength2D(), WithinRel(1.38515855107115149));
    REQUIRE_THAT(gpx.GetLength2D(), WithinRel(1.38515855107115149));
  }

  SECTION("computes 3D distances")
  {
    REQUIRE_THAT(gpx.tracks[0].segments[0].GetLength3D(), WithinRel(1.70840984883766467));
    REQUIRE_THAT(gpx.tracks[0].GetLength3D(), WithinRel(1.70840984883766467));
    REQUIRE_THAT(gpx.GetLength3D(), WithinRel(1.70840984883766467));
  }
}
