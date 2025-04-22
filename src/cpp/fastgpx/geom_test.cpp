#include <filesystem>
#include <functional>
#include <numeric>
#include <ranges>
#include <string>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "fastgpx/fastgpx.hpp"
#include "fastgpx/geom.hpp"
#include "fastgpx/test_data.hpp"

using Catch::Matchers::WithinAbs;

// Sufficient tolerance for comparing meters.
[[maybe_unused]] constexpr double kMETERS_TOL = 1e-4;

const auto project_path = std::filesystem::path(FASTGPX_PROJECT_DIR);

using GpxLengthFunc = const std::function<double(const fastgpx::LatLong&, const fastgpx::LatLong&)>;
static double GpxLength(const fastgpx::Gpx& gpx, const GpxLengthFunc& func)
{
  double distance = 0.0;
  for (const auto& track : gpx.tracks)
  {
    for (const auto& segment : track.segments)
    {
      const auto& points = segment.points;
      auto distances = std::views::zip(points, points | std::views::drop(1)) |
                       std::views::transform([&func](const auto& pair) {
                         return func(std::get<0>(pair), std::get<1>(pair));
                       });
      distance += std::accumulate(distances.begin(), distances.end(), 0.0);
    }
  }
  return distance;
}

static double fastgpx_distance2d(const fastgpx::LatLong& ll1, const fastgpx::LatLong& ll2)
{
  return fastgpx::v1::distance2d(ll1, ll2, false);
}

static double fastgpx_distance3d(const fastgpx::LatLong& ll1, const fastgpx::LatLong& ll2)
{
  return fastgpx::v1::distance3d(ll1, ll2, false);
}

TEST_CASE("Compute 2D distance", "[distance]")
{
  // ~380km route
  const auto path = project_path / "gpx/2024 TopCamp/Connected_20240518_094959_.gpx";
  const auto gpx = fastgpx::ParseGpx(path);
  // Reference lengths are based on the gpxpy implementations:
  const auto expected_haversine = GpxLength(gpx, fastgpx::v1::haversine);
  const auto expected_length2d = GpxLength(gpx, fastgpx_distance2d);
  const auto expected_length3d = GpxLength(gpx, fastgpx_distance3d);

  const auto kImplTolMeters = 400.0; // Accepted tolerance deviations between implementations.

  SECTION("v2 haversine")
  {
    double distance = GpxLength(gpx, fastgpx::v2::haversine);
    CHECK_THAT(distance, WithinAbs(expected_haversine, kImplTolMeters));
  };

  SECTION("v2 distance2d")
  {
    double distance = GpxLength(gpx, fastgpx::v2::distance2d);
    CHECK_THAT(distance, WithinAbs(expected_length2d, kImplTolMeters));
  };

  SECTION("v2 distance3d")
  {
    double distance = GpxLength(gpx, fastgpx::v2::distance3d);
    CHECK_THAT(distance, WithinAbs(expected_length3d, kImplTolMeters));
  };
}

static double fastgpx_distance2d_haversine(const fastgpx::LatLong& ll1, const fastgpx::LatLong& ll2)
{
  return fastgpx::v1::distance2d(ll1, ll2, true);
}

static double fastgpx_distance3d_haversine(const fastgpx::LatLong& ll1, const fastgpx::LatLong& ll2)
{
  return fastgpx::v1::distance3d(ll1, ll2, true);
}

TEST_CASE("Compute haversine distance", "[distance]")
{
  // ~380km route
  const auto path = project_path / "gpx/2024 TopCamp/Connected_20240518_094959_.gpx";
  const auto gpx = fastgpx::ParseGpx(path);

  const auto expected_haversine_2d = GpxLength(gpx, fastgpx::v2::haversine);
  const auto expected_haversine_3d = GpxLength(gpx, fastgpx::v2::haversine);

  const auto kImplTolMeters = 400.0; // Accepted tolerance deviations between implementations.

  SECTION("v2 distance2d with haversine")
  {
    double distance = GpxLength(gpx, fastgpx_distance2d_haversine);
    CHECK_THAT(distance, WithinAbs(expected_haversine_2d, kImplTolMeters));
  };

  SECTION("v2 distance3d with haversine")
  {
    double distance = GpxLength(gpx, fastgpx_distance3d_haversine);
    CHECK_THAT(distance, WithinAbs(expected_haversine_3d, kImplTolMeters));
  };
}

TEST_CASE("Benchmark distance", "[!benchmark][distance]")
{
  // ~380km route
  const auto path = project_path / "gpx/2024 TopCamp/Connected_20240518_094959_.gpx";
  const auto gpx = fastgpx::ParseGpx(path);

  BENCHMARK("gpxpy haversine")
  {
    return GpxLength(gpx, fastgpx::v1::haversine);
  };

  BENCHMARK("gpxpy distance2d")
  {
    return GpxLength(gpx, fastgpx_distance2d);
  };

  BENCHMARK("gpxpy distance3d")
  {
    return GpxLength(gpx, fastgpx_distance3d);
  };

  BENCHMARK("v2 haversine")
  {
    return GpxLength(gpx, fastgpx::v2::haversine);
  };

  BENCHMARK("v2 distance2d")
  {
    return GpxLength(gpx, fastgpx::v2::distance2d);
  };

  BENCHMARK("v2 distance3d")
  {
    return GpxLength(gpx, fastgpx::v2::distance3d);
  };
}
