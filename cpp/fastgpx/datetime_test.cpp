#include <chrono>
#include <format>
#include <string>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "fastgpx/datetime.hpp"

using std::chrono::system_clock;

std::string format_iso8601(const std::chrono::system_clock::time_point& tp)
{
  // return std::format("{:%Y-%m-%dT%H:%M:%SZ}", tp);

  // Convert the time_point to seconds precision to remove fractional seconds
  auto tp_seconds = std::chrono::time_point_cast<std::chrono::seconds>(tp);
  // Use std::format to format the time_point as ISO 8601 without fractional seconds
  return std::format("{:%Y-%m-%dT%H:%M:%SZ}", tp_seconds);
}
std::string format_iso8601(const std::chrono::utc_clock::time_point& tp)
{
  // return std::format("{:%Y-%m-%dT%H:%M:%SZ}", tp);

  // Convert the time_point to seconds precision to remove fractional seconds
  auto tp_seconds = std::chrono::time_point_cast<std::chrono::seconds>(tp);
  // Use std::format to format the time_point as ISO 8601 without fractional seconds
  return std::format("{:%Y-%m-%dT%H:%M:%SZ}", tp_seconds);
}

TEST_CASE("Parse iso8601 date string", "[datetime]")
{
  const std::string time_string = "2024-05-18T07:50:01Z";
  const auto expected_time = system_clock::from_time_t(1716015001);

  SECTION("v1 std::get_time")
  {
    const auto actual_time = fastgpx::v1::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);
    CHECK(format_iso8601(actual_time) == time_string);
  }

  SECTION("v2 std::chrono::parse")
  {
    const auto actual_time = fastgpx::v2::parse_iso8601(time_string);
    const auto expected_utc_time = std::chrono::utc_clock::from_sys(expected_time);
    CHECK(actual_time == expected_utc_time);
    CHECK(format_iso8601(actual_time) == time_string);
  }

  SECTION("v3 std::from_chars")
  {
    const auto actual_time = fastgpx::v3::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);
    CHECK(format_iso8601(actual_time) == time_string);
  }
}

TEST_CASE("Benchmark parse iso8601 date string", "[!benchmark][datetime]")
{
  const std::string time_string = "2024-05-18T06:50:01Z";

  BENCHMARK("v1 std::get_time") { return fastgpx::v1::parse_iso8601(time_string); };
  BENCHMARK("v2 std::chrono::parse") { return fastgpx::v2::parse_iso8601(time_string); };
  BENCHMARK("v3 std::from_chars") { return fastgpx::v3::parse_iso8601(time_string); };
}
