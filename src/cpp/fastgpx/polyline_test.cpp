#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "fastgpx/errors.hpp"
#include "fastgpx/fastgpx.hpp"
#include "fastgpx/polyline.hpp"

using fastgpx::LatLong;
using fastgpx::polyline::Precision;

TEST_CASE("polyline encode Google example", "[polyline]")
{
  // https://developers.google.com/maps/documentation/utilities/polylinealgorithm
  const std::vector<LatLong> points = {
      {38.5, -120.2, 0.0},
      {40.7, -120.95, 0.0},
      {43.252, -126.453, 0.0},
  };
  CHECK(fastgpx::polyline::encode(points, Precision::Five) == "_p~iF~ps|U_ulLnnqC_mqNvxq`@");
}

TEST_CASE("polyline encode/decode round trip extreme coordinates", "[polyline]")
{
  const std::vector<LatLong> points = {
      {-90.0, -180.0, 0.0},
      {90.0, 180.0, 0.0},
      {-90.0, -180.0, 0.0},
      {0.0, 0.0, 0.0},
  };
  for (const auto precision : {Precision::Five, Precision::Six})
  {
    CAPTURE(static_cast<int>(precision));
    const auto encoded = fastgpx::polyline::encode(points, precision);
    const auto decoded = fastgpx::polyline::decode(encoded, precision);
    CHECK(decoded == points);
  }
}

TEST_CASE("polyline encode rejects coordinates it cannot represent", "[polyline]")
{
  // Found while writing fuzz_polyline_encode (#49). Casting NaN, infinity or an out-of-range
  // double to int is undefined behaviour, so such values must be rejected before the cast. The
  // limits match the decoder: |latitude| <= 90 and |longitude| <= 180 after rounding.
  constexpr double nan = std::numeric_limits<double>::quiet_NaN();
  constexpr double inf = std::numeric_limits<double>::infinity();

  SECTION("non-finite")
  {
    CHECK_THROWS_AS(fastgpx::polyline::encode(std::vector<LatLong>{{nan, 0.0, 0.0}}),
                    fastgpx::value_error);
    CHECK_THROWS_AS(fastgpx::polyline::encode(std::vector<LatLong>{{0.0, inf, 0.0}}),
                    fastgpx::value_error);
    CHECK_THROWS_AS(fastgpx::polyline::encode(std::vector<LatLong>{{-inf, 0.0, 0.0}}),
                    fastgpx::value_error);
  }

  SECTION("out of range")
  {
    CHECK_THROWS_AS(fastgpx::polyline::encode(std::vector<LatLong>{{90.1, 0.0, 0.0}}),
                    fastgpx::value_error);
    CHECK_THROWS_AS(fastgpx::polyline::encode(std::vector<LatLong>{{0.0, -180.1, 0.0}}),
                    fastgpx::value_error);
    // Would overflow the int cast outright.
    CHECK_THROWS_AS(fastgpx::polyline::encode(std::vector<LatLong>{{1e300, 0.0, 0.0}}),
                    fastgpx::value_error);
    CHECK_THROWS_AS(fastgpx::polyline::encode(std::vector<LatLong>{{0.0, -1e300, 0.0}}),
                    fastgpx::value_error);
  }

  SECTION("rounding decides at the boundary")
  {
    // 90.000004 rounds to 90.00000 at precision 5; 90.000006 rounds past it.
    CHECK_NOTHROW(
        fastgpx::polyline::encode(std::vector<LatLong>{{90.000004, -180.000004, 0.0}}));
    CHECK_THROWS_AS(fastgpx::polyline::encode(std::vector<LatLong>{{90.000006, 0.0, 0.0}}),
                    fastgpx::value_error);
    // At precision 6 the same value is a genuine 90.000006 and must be rejected.
    CHECK_THROWS_AS(
        fastgpx::polyline::encode(std::vector<LatLong>{{90.000004, 0.0, 0.0}}, Precision::Six),
        fastgpx::value_error);
  }

  SECTION("error message names the coordinate")
  {
    CHECK_THROWS_WITH(fastgpx::polyline::encode(std::vector<LatLong>{{91.0, 0.0, 0.0}}),
                      Catch::Matchers::ContainsSubstring("latitude out of range"));
    CHECK_THROWS_WITH(fastgpx::polyline::encode(std::vector<LatLong>{{0.0, 181.0, 0.0}}),
                      Catch::Matchers::ContainsSubstring("longitude out of range"));
  }
}

TEST_CASE("Benchmark polyline encode/decode", "[!benchmark][polyline]")
{
  // A synthetic 10k point track with small, realistic deltas between consecutive points.
  std::vector<LatLong> points;
  points.reserve(10'000);
  for (std::size_t i = 0; i < 10'000; ++i)
  {
    const double t = static_cast<double>(i);
    points.push_back({63.0 + 0.0001 * t + 0.00001 * std::sin(t), 10.0 + 0.0002 * t, 0.0});
  }
  const auto encoded5 = fastgpx::polyline::encode(points, Precision::Five);
  const auto encoded6 = fastgpx::polyline::encode(points, Precision::Six);

  BENCHMARK("encode 10k points, precision 5")
  {
    return fastgpx::polyline::encode(points, Precision::Five);
  };
  BENCHMARK("encode 10k points, precision 6")
  {
    return fastgpx::polyline::encode(points, Precision::Six);
  };
  BENCHMARK("decode 10k points, precision 5")
  {
    return fastgpx::polyline::decode(encoded5, Precision::Five);
  };
  BENCHMARK("decode 10k points, precision 6")
  {
    return fastgpx::polyline::decode(encoded6, Precision::Six);
  };
}
