#include "fastgpx.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Generators::table;
using Catch::Matchers::WithinRel;

const auto project_path = std::filesystem::path(FASTGPX_PROJECT_DIR);

TEST_CASE("Parse two-point single segment track", "[parse][simple]")
{
  const auto path = project_path / "gpx/test/debug-segment.gpx";
  const auto gpx = fastgpx::ParseGpx(path);

  REQUIRE(gpx.tracks.size() == 1);
  REQUIRE(gpx.tracks[0].segments.size() == 1);

  CHECK_THAT(gpx.tracks[0].segments[0].GetLength2D(), WithinRel(1.38515855107115149));
  CHECK_THAT(gpx.tracks[0].GetLength2D(), WithinRel(1.38515855107115149));
  CHECK_THAT(gpx.GetLength2D(), WithinRel(1.38515855107115149));

  CHECK_THAT(gpx.tracks[0].segments[0].GetLength3D(), WithinRel(1.70840984883766467));
  CHECK_THAT(gpx.tracks[0].GetLength3D(), WithinRel(1.70840984883766467));
  CHECK_THAT(gpx.GetLength3D(), WithinRel(1.70840984883766467));
}

struct ExpectedSegment
{
  double length2d; // Meters
  double length3d; // Meters
};

struct ExpectedTrack
{
  std::vector<ExpectedSegment> segments;
  double length2d; // Meters
  double length3d; // Meters
};

struct ExpectedGpx
{
  std::vector<ExpectedTrack> tracks;
  double length2d; // Meters
  double length3d; // Meters
};

TEST_CASE("Parse real world GPX files", "[parse][real_world]")
{
  SECTION("Matches expected values")
  {
    std::string gpx_path_name;
    ExpectedGpx expected;
    std::tie(gpx_path_name, expected) =
        GENERATE(table<std::string, ExpectedGpx>(
            {
                std::make_tuple(
                    "gpx/2024 TopCamp/Connected_20240518_094959_.gpx",
                    ExpectedGpx{
                        .tracks = {
                            ExpectedTrack{
                                .segments = {
                                    // 0
                                    ExpectedSegment{
                                        .length2d = {17824.19175882741183159},
                                        .length3d = {17859.48257500379986595},
                                    },
                                    // 1
                                    ExpectedSegment{
                                        .length2d = {10061.19192487031432393},
                                        .length3d = {10112.42294839789610705},
                                    },
                                    // 2
                                    ExpectedSegment{
                                        .length2d = {98386.76161200449860189},
                                        .length3d = {98501.43746740205097012},
                                    },
                                    // 3
                                    ExpectedSegment{
                                        .length2d = {102427.57726620219182223},
                                        .length3d = {102504.92180776111490559},
                                    },
                                    // 4
                                    ExpectedSegment{
                                        .length2d = {1118.01310707499260388},
                                        .length3d = {1148.03872186123771826},
                                    },
                                    // 5
                                    ExpectedSegment{
                                        .length2d = {6833.24760896845145908},
                                        .length3d = {6835.74003269745844591},
                                    },
                                    // 6
                                    ExpectedSegment{
                                        .length2d = {44184.33701133414433571},
                                        .length3d = {44217.56724880077672424},
                                    },
                                    // 7
                                    ExpectedSegment{
                                        .length2d = {67033.87632726262381766},
                                        .length3d = {67071.42442790411587339},
                                    },
                                    // 8
                                    ExpectedSegment{
                                        .length2d = {35404.20735848945332691},
                                        .length3d = {35434.91161590197589248},
                                    },
                                },
                                .length2d = 383273.40397503407439217,
                                .length3d = 383685.94684573041740805,
                            },
                        },
                        .length2d = 383273.40397503407439217,
                        .length3d = 383685.94684573041740805,
                    }),
            }));

    const std::filesystem::path path(gpx_path_name);
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

        CHECK_THAT(segment.GetLength2D(), WithinRel(expected_segment.length2d));
        CHECK_THAT(segment.GetLength3D(), WithinRel(expected_segment.length3d));
      }

      CHECK_THAT(track.GetLength2D(), WithinRel(expected_track.length2d));
      CHECK_THAT(track.GetLength3D(), WithinRel(expected_track.length3d));
    }

    CHECK_THAT(gpx.GetLength2D(), WithinRel(expected.length2d));
    CHECK_THAT(gpx.GetLength3D(), WithinRel(expected.length3d));
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
