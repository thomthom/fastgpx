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

TEST_CASE("Parse Last millisecond of May 18, 2024", "[datetime]")
{
  // Timestamp                  1716076799
  // Timestamp in milliseconds  1716076799999
  // ISO 8601                   2024-05-18T23:59:59.999Z
  // Date Time (UTC)            18 May 2024, 23:59:59

  const std::string time_string = "2024-05-18T23:59:59.999Z";
  const std::time_t expected_timestamp = 1716076799;
  const std::time_t expected_timestamp_ms = 1716076799999;
  const std::chrono::milliseconds millis_duration(expected_timestamp_ms);
  const auto expected_time = std::chrono::system_clock::time_point(millis_duration);
  const auto expected_formatted = format_iso8601(expected_time);

  CAPTURE(time_string, expected_formatted, expected_timestamp, expected_timestamp_ms,
          expected_time);

  SECTION("v3 std::chrono::parse system_clock")
  {
    const auto actual_time = fastgpx::v3::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_formatted);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
}

TEST_CASE("138th day of 2024 (May 17) at specific time", "[datetime]")
{
  // https://dencode.com/en/date/iso8601

  // ISO8601 Date               20240517T095001+0200
  // ISO8601 Date (Extend)      2024-05-17T09:50:01+02:00
  // ISO8601 Date (Week)        2024-W20-5T09:50:01+02:00
  // ISO8601 Date (Ordinal)     2024-138T09:50:01+02:00

  // Timestamp                  1715932201
  // Timestamp in milliseconds  1715932201000
  // ISO 8601                   2024-05-17T07:50:01.000Z
  // Date Time (UTC)            17 May 2024, 07:50:01

  const std::string time_string = "2024-138T07:50:01Z";
  const std::time_t expected_timestamp = 1715932201;
  const std::time_t expected_timestamp_ms = 1715932201000;
  const std::chrono::milliseconds millis_duration(expected_timestamp_ms);
  const auto expected_time = std::chrono::system_clock::time_point(millis_duration);
  const auto expected_formatted = format_iso8601(expected_time);

  CAPTURE(time_string, expected_formatted, expected_timestamp, expected_timestamp_ms,
          expected_time);

  SECTION("v3 std::chrono::parse system_clock")
  {
    const auto actual_time = fastgpx::v3::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_formatted);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
}

TEST_CASE("Parse time bounds of Connected_20240518_094959_.gpx", "[datetime]")
{
  // https://www.timestamp-converter.com/

  // Timestamp                  1716050778
  // Timestamp in milliseconds  1716050778000
  // ISO 8601                   2024-05-18T16:46:18.000Z
  // Date Time (UTC)            May 18, 2024, 4:46:18 PM

  const std::string time_string = "2024-05-18T16:46:18Z";
  const std::time_t expected_timestamp = 1716050778;
  const std::time_t expected_timestamp_ms = 1716050778000;
  const std::chrono::milliseconds millis_duration(expected_timestamp_ms);
  const auto expected_time = std::chrono::system_clock::time_point(millis_duration);
  const auto expected_formatted = format_iso8601(expected_time);

  CAPTURE(time_string, expected_formatted, expected_timestamp, expected_timestamp_ms,
          expected_time);

  SECTION("v3 std::chrono::parse system_clock")
  {
    const auto actual_time = fastgpx::v3::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_formatted);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
}

TEST_CASE("Parse multiple iso8601 date strings with generators", "[datetime][generated]")
{
  auto [time_string, expected_timestamp_ms, description] = GENERATE(
      // Basic date-time format (YYYY-MM-DDTHH:MM:SSZ)
      std::tuple{"2024-05-18T07:50:01Z", 1716018601000LL, "Standard date-time in UTC"},
      std::tuple{"2023-12-25T00:00:00Z", 1708819200000LL, "Midnight on Christmas in UTC"},
      std::tuple{"2022-02-28T23:59:59Z", 1646092799000LL,
                 "Last second of February 28th in non-leap year"},

      // Including milliseconds (YYYY-MM-DDTHH:MM:SS.sssZ)
      std::tuple{"2024-05-18T07:50:01.123Z", 1716018601123LL, "Milliseconds included"},
      std::tuple{"2024-05-18T07:50:01.000Z", 1716018601000LL, "Exact time with zero milliseconds"},
      std::tuple{"2024-05-18T23:59:59.999Z", 1716076799999LL,
                 "Last millisecond before midnight UTC"},

      // Including microseconds (YYYY-MM-DDTHH:MM:SS.ssssssZ)
      std::tuple{"2024-05-18T07:50:01.123456Z", 1716018601123LL,
                 "Microseconds truncated to milliseconds"},
      std::tuple{"2024-05-18T07:50:01.987654Z", 1716018601987LL,
                 "Different microseconds, rounded to milliseconds"},

      // Leap year and month boundaries
      std::tuple{"2024-02-29T12:30:45Z", 1709206245000LL, "Leap day in 2024 at specific time"},
      std::tuple{"2023-03-01T00:00:01Z", 1677628801000LL,
                 "Just after midnight, March 1 (non-leap year)"},
      std::tuple{"2024-12-31T23:59:59Z", 1735689599000LL, "Last second of the year 2024"},
      std::tuple{"2024-01-01T00:00:00Z", 1704067200000LL, "Midnight of New Year's Day, 2024"},

      // Variable fractional seconds precision (fractions should be ignored)
      std::tuple{"2024-05-18T07:50:01.1Z", 1716018601000LL,
                 "1 decimal place, truncated to milliseconds"},
      std::tuple{"2024-05-18T07:50:01.12Z", 1716018601000LL,
                 "2 decimal places, truncated to milliseconds"},
      std::tuple{"2024-05-18T07:50:01.1234Z", 1716018601123LL,
                 "4 decimal places, truncated to milliseconds"},
      std::tuple{"2024-05-18T07:50:01.123456789Z", 1716018601123LL,
                 "9 decimal places, truncated to milliseconds"},

      // Basic format without separators (YYYYMMDDTHHMMSSZ)
      std::tuple{"20240518T075001Z", 1716018601000LL, "Date-time in compact form"},
      std::tuple{"20240518T075001.123Z", 1716018601123LL, "Compact form with milliseconds"},

      // Week dates (YYYY-Www-DTHH:MM:SSZ)
      /*
      std::tuple{"2024-W20-6T07:50:01Z", 1716018601000LL,
                 "Week date format, Saturday of 20th week of 2024"},
      std::tuple{"2024-W01-1T12:00:00Z", 1704196800000LL, "First day of the first week of 2024 at
      noon"},
      */

      // Ordinal dates (YYYY-DDDTHH:MM:SSZ)
      std::tuple{"2024-138T07:50:01Z", 1715932201000LL,
                 "138th day of 2024 (May 17) at specific time"},
      std::tuple{"2024-001T00:00:00Z", 1704067200000LL, "First day of 2024 at midnight"},

      // Midnight times
      std::tuple{"2024-05-18T00:00:00Z", 1715990400000LL, "Midnight of May 18, 2024"},
      std::tuple{"2024-05-18T23:59:59.999Z", 1716076799999LL, "Last millisecond of May 18, 2024"});

  const auto expected_timestamp = static_cast<time_t>(expected_timestamp_ms / 1000);

  const std::chrono::milliseconds millis_duration(expected_timestamp_ms);
  const auto expected_time = std::chrono::system_clock::time_point(millis_duration);

  const auto expected_formatted = format_iso8601(expected_time);

  CAPTURE(description, time_string, expected_formatted, expected_timestamp_ms, expected_timestamp,
          expected_time);
  /*
  SECTION("v1 std::get_time")
  {
    const auto actual_time = fastgpx::v1::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_formatted);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
  */

  SECTION("v3 std::chrono::parse system_clock")
  {
    const auto actual_time = fastgpx::v3::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_formatted);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }

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
