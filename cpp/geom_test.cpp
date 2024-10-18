#include <filesystem>
#include <functional>
#include <numeric>
#include <ranges>
#include <string>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_test_macros.hpp>

#include "geom.hpp"
#include "fastgpx.hpp"
#include "test_data.hpp"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

// Sufficient tolerance for comparing meters.
constexpr double kMETERS_TOL = 1e-4;

const auto project_path = std::filesystem::path(FASTGPX_PROJECT_DIR);

TEST_CASE("Compute 2D distance", "[distance]")
{
  const auto path = project_path / "gpx/2024 TopCamp/Connected_20240518_094959_.gpx";
  const auto gpx = fastgpx::ParseGpx(path);
  const auto expected_length2d = gpx.GetLength2D();
  const auto expected_length3d = gpx.GetLength3D();

  SECTION("gpxpy haversine")
  {
    double distance = 0.0;
    for (const auto &track : gpx.tracks)
    {
      for (const auto &segment : track.segments)
      {
        const auto &points = segment.points;
        auto distances = std::views::zip(points, points | std::views::drop(1)) |
                         std::views::transform([](const auto &pair)
                                               { return fastgpx::haversine(std::get<0>(pair), std::get<1>(pair)); });
        distance += std::accumulate(distances.begin(), distances.end(), 0.0);
      }
    }
    CHECK_THAT(distance, WithinAbs(expected_length2d, kMETERS_TOL));
    CHECK_THAT(distance, WithinRel(expected_length2d, kMETERS_TOL));
    CHECK_THAT(distance, WithinRel(expected_length2d));

    CHECK_THAT(distance, WithinAbs(expected_length3d, kMETERS_TOL));
    CHECK_THAT(distance, WithinRel(expected_length3d, kMETERS_TOL));
    CHECK_THAT(distance, WithinRel(expected_length3d));
  };
}

using GpxLengthFunc = const std::function<double(const fastgpx::LatLong &, const fastgpx::LatLong &)>;
static double GpxLength(const fastgpx::Gpx &gpx, const GpxLengthFunc &func)
{
  double distance = 0.0;
  for (const auto &track : gpx.tracks)
  {
    for (const auto &segment : track.segments)
    {
      const auto &points = segment.points;
      auto distances = std::views::zip(points, points | std::views::drop(1)) |
                       std::views::transform([&func](const auto &pair)
                                             { return func(std::get<0>(pair), std::get<1>(pair)); });
      distance += std::accumulate(distances.begin(), distances.end(), 0.0);
    }
  }
  return distance;
}

static double fastgpx_distance2d(const fastgpx::LatLong &ll1, const fastgpx::LatLong &ll2)
{
  return fastgpx::distance2d(ll1, ll2, false);
}

TEST_CASE("Benchmark distance", "[!benchmark][distance]")
{
  const auto path = project_path / "gpx/2024 TopCamp/Connected_20240518_094959_.gpx";
  const auto gpx = fastgpx::ParseGpx(path);

  BENCHMARK("gpxpy distance2d")
  {
    return GpxLength(gpx, fastgpx_distance2d);
  };

  BENCHMARK("gpxpy haversine")
  {
    return GpxLength(gpx, fastgpx::haversine);
  };
}
