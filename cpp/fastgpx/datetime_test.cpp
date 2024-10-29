#include <chrono>
#include <string>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "fastgpx/datetime.hpp"

using std::chrono::system_clock;

TEST_CASE("Parse iso8601 date string", "[datetime]")
{
  const std::string time_string = "2024-05-18T07:50:01Z";
  const auto expected_time = system_clock::from_time_t(1716015001);

  SECTION("v1")
  {
    const auto actual_time = fastgpx::v1::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);
  }
}

TEST_CASE("Benchmark parse iso8601 date string", "[!benchmark][datetime]")
{
  const std::string time_string = "2024-05-18T06:50:01Z";

  BENCHMARK("v1 parse_iso8601") { return fastgpx::v1::parse_iso8601(time_string); };
}
