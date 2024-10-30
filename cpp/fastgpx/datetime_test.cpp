#include <chrono>
#include <format>
#include <string>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

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

TEST_CASE("Parse multiple iso8601 date strings with generators", "[datetime][generated]")
{
  auto [time_string, expected_timestamp_int, description] = GENERATE(
      // Basic date-time format (YYYY-MM-DDTHH:MM:SSZ)
      std::tuple{"2024-05-18T07:50:01Z", 1716018601, "Standard date-time in UTC"},
      std::tuple{"2023-12-25T00:00:00Z", 1708819200, "Midnight on Christmas in UTC"},
      std::tuple{"2022-02-28T23:59:59Z", 1646092799,
                 "Last second of February 28th in non-leap year"},

      // Including milliseconds (YYYY-MM-DDTHH:MM:SS.sssZ)
      std::tuple{"2024-05-18T07:50:01.123Z", 1716018601, "Milliseconds ignored"},
      std::tuple{"2024-05-18T07:50:01.000Z", 1716018601, "Exact time with zero milliseconds"},
      std::tuple{"2024-05-18T23:59:59.999Z", 1716076799, "Last millisecond before midnight UTC"},

      // Including microseconds (YYYY-MM-DDTHH:MM:SS.ssssssZ)
      std::tuple{"2024-05-18T07:50:01.123456Z", 1716018601, "Microseconds ignored"},
      std::tuple{"2024-05-18T07:50:01.987654Z", 1716018601,
                 "Different microseconds, same timestamp"},

      // Leap year and month boundaries
      std::tuple{"2024-02-29T12:30:45Z", 1709206245, "Leap day in 2024 at specific time"},
      std::tuple{"2023-03-01T00:00:01Z", 1677628801,
                 "Just after midnight, March 1 (non-leap year)"},
      std::tuple{"2024-12-31T23:59:59Z", 1735689599, "Last second of the year 2024"},
      std::tuple{"2024-01-01T00:00:00Z", 1704067200, "Midnight of New Year's Day, 2024"},

      // Variable fractional seconds precision (fractions should be ignored)
      std::tuple{"2024-05-18T07:50:01.1Z", 1716018601, "1 decimal place"},
      std::tuple{"2024-05-18T07:50:01.12Z", 1716018601, "2 decimal places"},
      std::tuple{"2024-05-18T07:50:01.1234Z", 1716018601, "4 decimal places"},
      std::tuple{"2024-05-18T07:50:01.123456789Z", 1716018601, "9 decimal places, truncated"},

      // Basic format without separators (YYYYMMDDTHHMMSSZ)
      std::tuple{"20240518T075001Z", 1716018601, "Date-time in compact form"},
      std::tuple{"20240518T075001.123Z", 1716018601, "Compact form with milliseconds"},

      // Week dates (YYYY-Www-DTHH:MM:SSZ)
      /*
      std::tuple{"2024-W20-6T07:50:01Z", 1716018601,
                 "Week date format, Saturday of 20th week of 2024"},
      std::tuple{"2024-W01-1T12:00:00Z", 1704196800, "First day of the first week of 2024 at noon"},
      */

      // Ordinal dates (YYYY-DDDTHH:MM:SSZ)
      std::tuple{"2024-138T07:50:01Z", 1716018601, "138th day of 2024 (May 18) at specific time"},
      std::tuple{"2024-001T00:00:00Z", 1704067200, "First day of 2024 at midnight"},

      // Midnight times
      std::tuple{"2024-05-18T00:00:00Z", 1715990400, "Midnight of May 18, 2024"},
      std::tuple{"2024-05-18T23:59:59.999Z", 1716076799, "Last millisecond of May 18, 2024"});

  const time_t expected_timestamp = expected_timestamp_int;
  const auto expected_time = std::chrono::system_clock::from_time_t(expected_timestamp);
  const auto expected_formatted = format_iso8601(expected_time);

  CAPTURE(description, time_string, expected_formatted, expected_timestamp, expected_time);

  SECTION("v1 std::get_time")
  {
    const auto actual_time = fastgpx::v1::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_formatted);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
  /*
    SECTION("v3 std::chrono::parse system_clock")
    {
      const auto actual_time = fastgpx::v3::parse_iso8601(time_string);
      CHECK(actual_time == expected_time);

      CHECK(format_iso8601(actual_time) == expected_formatted);

      const auto actual_timestamp = time_point_to_epoch(actual_time);
      CHECK(actual_timestamp == expected_timestamp);
    }
   */
  /*
    SECTION("v4 std::from_chars")
    {
      const auto actual_time = fastgpx::v4::parse_iso8601(time_string);
      CHECK(actual_time == expected_time);

      CHECK(format_iso8601(actual_time) == expected_formatted);

      const auto actual_timestamp = time_point_to_epoch(actual_time);
      CHECK(actual_timestamp == expected_timestamp);
    }
  */
}

TEST_CASE("Benchmark parse iso8601 date string", "[!benchmark][datetime]")
{
  const std::string time_string = "2024-05-18T06:50:01Z";

  BENCHMARK("v1 std::get_time") { return fastgpx::v1::parse_iso8601(time_string); };
  BENCHMARK("v2 std::chrono::parse") { return fastgpx::v2::parse_iso8601(time_string); };
  BENCHMARK("v3 std::chrono::parse") { return fastgpx::v3::parse_iso8601(time_string); };
  BENCHMARK("v4 std::from_chars") { return fastgpx::v4::parse_iso8601(time_string); };
}
