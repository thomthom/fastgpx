#include <chrono>
#include <format>
#include <string>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "fastgpx/datetime.hpp"

std::string format_iso8601(const std::chrono::system_clock::time_point& tp)
{
  const auto tp_seconds = std::chrono::floor<std::chrono::seconds>(tp);
  return std::format("{:%Y-%m-%dT%H:%M:%SZ}", tp_seconds);
}

std::string format_iso8601(const std::chrono::utc_clock::time_point& tp)
{
  const auto tp_seconds = std::chrono::floor<std::chrono::seconds>(tp);
  return std::format("{:%Y-%m-%dT%H:%M:%SZ}", tp_seconds);
}

auto time_point_to_epoch(const std::chrono::system_clock::time_point& tp)
{
  return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
}

auto time_point_to_epoch(const std::chrono::utc_clock::time_point& tp)
{
  // The time_since_epoch isn't returning expected UNIX epoch for utc_clock.
  // For an ISO8601 time/date string representing 1716018601 the utc_clock
  // will return 1716018628.
  const auto sys_tp = std::chrono::utc_clock::to_sys(tp);
  return time_point_to_epoch(sys_tp);
}

TEST_CASE("Parse iso8601 date string", "[datetime]")
{
  // "2024-05-18T07:50:01Z"
  // https://www.timestamp-converter.com/
  //
  // Timestamp                   1716018601
  // Timestamp in milliseconds   1716018601000
  // ISO 8601                    2024-05-18T07:50:01.000Z
  // Date Time (UTC)             18 May 2024, 07:50:01
  // Date Time (your time zone)  18 May 2024, 09:50:01

  const std::string time_string = "2024-05-18T07:50:01Z";
  const std::time_t expected_timestamp = 1716018601;
  const auto expected_time = std::chrono::system_clock::from_time_t(expected_timestamp);

  SECTION("v1 std::get_time")
  {
    const auto actual_time = fastgpx::v1::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }

  SECTION("v2 std::chrono::parse utc_clock")
  {
    // Timestamp	1716018628
    // Timestamp in milliseconds	1716018628000
    // ISO 8601	2024-05-18T07:50:28.000Z
    // Date Time (UTC)	18 May 2024, 07:50:28
    // Date Time (your time zone)	18 May 2024, 09:50:28

    const auto expected_utc_time = std::chrono::utc_clock::from_sys(expected_time);

    const auto actual_time = fastgpx::v2::parse_iso8601(time_string);
    CHECK(actual_time == expected_utc_time);

    CHECK(format_iso8601(actual_time) == time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }

  SECTION("v3 std::chrono::parse system_clock")
  {
    const auto actual_time = fastgpx::v3::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }

  SECTION("v4 std::from_chars")
  {
    const auto actual_time = fastgpx::v4::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
}

TEST_CASE("Benchmark parse iso8601 date string", "[!benchmark][datetime]")
{
  const std::string time_string = "2024-05-18T06:50:01Z";

  BENCHMARK("v1 std::get_time") { return fastgpx::v1::parse_iso8601(time_string); };
  BENCHMARK("v2 std::chrono::parse") { return fastgpx::v2::parse_iso8601(time_string); };
  BENCHMARK("v3 std::chrono::parse") { return fastgpx::v3::parse_iso8601(time_string); };
  BENCHMARK("v4 std::from_chars") { return fastgpx::v4::parse_iso8601(time_string); };
}
