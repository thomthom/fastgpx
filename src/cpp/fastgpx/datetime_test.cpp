#include <chrono>
#include <format>
#include <string>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "fastgpx/datetime.hpp"
#include "fastgpx/errors.hpp"

template<typename TIME = std::chrono::seconds>
std::string format_iso8601(const std::chrono::system_clock::time_point& tp)
{
  const auto tp_seconds = std::chrono::floor<TIME>(tp);
  return std::format("{:%Y-%m-%dT%H:%M:%SZ}", tp_seconds);
}

template<typename TIME = std::chrono::seconds>
auto time_point_to_epoch(const std::chrono::system_clock::time_point& tp)
{
  return std::chrono::duration_cast<TIME>(tp.time_since_epoch()).count();
}

TEST_CASE("Parse iso8601 extended time YYYY-MM-DDThh:mm:ssZ", "[datetime][gpxtime]")
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

  CAPTURE(time_string, expected_timestamp, expected_time);

  const auto actual_time = fastgpx::parse_gpx_time(time_string);
  CHECK(actual_time == expected_time);

  CHECK(format_iso8601(actual_time) == time_string);

  const auto actual_timestamp = time_point_to_epoch(actual_time);
  CHECK(actual_timestamp == expected_timestamp);
}

TEST_CASE("Parse iso8601 extended time YYYY-MM-DDThh:mm:ss.sssZ", "[datetime][gpxtime]")
{
  // "2024-11-17T06:54:12.123Z"
  // https://www.timestamp-converter.com/

  const std::string time_string = "2024-11-17T06:54:12.123Z";
  const std::string expected_time_string = "2024-11-17T06:54:12.123Z";
  const std::time_t expected_timestamp = 1731826452;
  const std::time_t expected_timestamp_ms = 1731826452123;
  const auto expected_time =
      std::chrono::system_clock::time_point(std::chrono::milliseconds(expected_timestamp_ms));

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  const auto actual_time = fastgpx::parse_gpx_time(time_string);
  CHECK(actual_time == expected_time);

  CHECK(format_iso8601<std::chrono::milliseconds>(actual_time) == expected_time_string);

  const auto actual_timestamp = time_point_to_epoch<std::chrono::milliseconds>(actual_time);
  CHECK(actual_timestamp == expected_timestamp_ms);
}

TEST_CASE("Parse iso8601 extended date time positive timezone", "[datetime][gpxtime]")
{
  // "2024-11-17T06:14:13+08:30"
  // https://www.timestamp-converter.com/

  // Timezone chosen to "underflow" TZ hour and minute.
  const std::string time_string = "2024-11-17T06:14:13+08:30";
  const std::string expected_time_string = "2024-11-16T21:44:13Z";
  const std::time_t expected_timestamp = 1731793453;
  const auto expected_time = std::chrono::system_clock::from_time_t(expected_timestamp);

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  const auto actual_time = fastgpx::parse_gpx_time(time_string);
  CHECK(actual_time == expected_time);

  CHECK(format_iso8601(actual_time) == expected_time_string);

  const auto actual_timestamp = time_point_to_epoch(actual_time);
  CHECK(actual_timestamp == expected_timestamp);
}

TEST_CASE("Parse iso8601 extended date time negative timezone", "[datetime][gpxtime]")
{
  // "2024-11-17T06:54:43-08:30"
  // https://www.timestamp-converter.com/

  // Timezone chosen to "overflow" TZ hour and minute.
  const std::string time_string = "2024-11-17T06:54:43-08:30";
  const std::string expected_time_string = "2024-11-17T15:24:43Z";
  const std::time_t expected_timestamp = 1731857083;
  const auto expected_time = std::chrono::system_clock::from_time_t(expected_timestamp);

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  const auto actual_time = fastgpx::parse_gpx_time(time_string);
  CHECK(actual_time == expected_time);

  CHECK(format_iso8601(actual_time) == expected_time_string);

  const auto actual_timestamp = time_point_to_epoch(actual_time);
  CHECK(actual_timestamp == expected_timestamp);
}

TEST_CASE("Parse iso8601 extended date time milliseconds positive timezone", "[datetime][gpxtime]")
{
  // "2024-11-17T06:14:13.123+08:30"
  // https://www.timestamp-converter.com/

  // Timezone chosen to "underflow" TZ hour and minute.
  const std::string time_string = "2024-11-17T06:14:13.123+08:30";
  const std::string expected_time_string = "2024-11-16T21:44:13.123Z";
  const std::time_t expected_timestamp = 1731793453;
  const std::time_t expected_timestamp_ms = 1731793453123;
  const auto expected_time =
      std::chrono::system_clock::time_point(std::chrono::milliseconds(expected_timestamp_ms));

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  const auto actual_time = fastgpx::parse_gpx_time(time_string);
  CHECK(actual_time == expected_time);

  CHECK(format_iso8601<std::chrono::milliseconds>(actual_time) == expected_time_string);

  const auto actual_timestamp = time_point_to_epoch<std::chrono::milliseconds>(actual_time);
  CHECK(actual_timestamp == expected_timestamp_ms);
}

TEST_CASE("Parse iso8601 extended date time milliseconds negative timezone", "[datetime][gpxtime]")
{
  // "2024-11-17T06:54:43.123-08:30"
  // https://www.timestamp-converter.com/

  // Timezone chosen to "overflow" TZ hour and minute.
  const std::string time_string = "2024-11-17T06:54:43.123-08:30";
  const std::string expected_time_string = "2024-11-17T15:24:43.123Z";
  const std::time_t expected_timestamp = 1731857083;
  const std::time_t expected_timestamp_ms = 1731857083123;
  const auto expected_time =
      std::chrono::system_clock::time_point(std::chrono::milliseconds(expected_timestamp_ms));

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  const auto actual_time = fastgpx::parse_gpx_time(time_string);
  CHECK(actual_time == expected_time);

  CHECK(format_iso8601<std::chrono::milliseconds>(actual_time) == expected_time_string);

  const auto actual_timestamp = time_point_to_epoch<std::chrono::milliseconds>(actual_time);
  CHECK(actual_timestamp == expected_timestamp_ms);
}

TEST_CASE("Parse GPX time missing timezone YYYY-MM-DDThh:mm:ss", "[datetime][gpxtime]")
{
  // "2024-11-17T06:54:12"
  // https://www.timestamp-converter.com/

  // Interpret the string as UTC regardless of missing 'Z', this isn't what the
  // ISO8601 standard describe but it is what the GPX standard describe.
  //
  // ISO8601 standard:
  // > If no UTC relation information is given with a time representation, the
  // > time is assumed to be in local time. While it may be safe to assume local
  // > time when communicating in the same time zone, it is ambiguous when used
  // > in communicating across different time zones.
  //
  // GPX 1.1 standard:
  // > Creation/modification timestamp for element. Date and time in are in
  // > Univeral Coordinated Time (UTC), not local time! Conforms to ISO 8601
  // > specification for date/time representation. Fractional seconds are
  // > allowed for millisecond timing in tracklogs.
  const std::string time_string = "2024-11-17T06:54:12";
  const std::string expected_time_string = "2024-11-17T06:54:12Z";
  const std::time_t expected_timestamp = 1731826452;
  const auto expected_time = std::chrono::system_clock::from_time_t(expected_timestamp);

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  const auto actual_time = fastgpx::parse_gpx_time(time_string);
  CHECK(actual_time == expected_time);

  CHECK(format_iso8601(actual_time) == expected_time_string);

  const auto actual_timestamp = time_point_to_epoch(actual_time);
  CHECK(actual_timestamp == expected_timestamp);
}

TEST_CASE("Parse GPX time missing timezone YYYY-MM-DDThh:mm:ss.sss", "[datetime][gpxtime]")
{
  // "2024-11-17T06:54:12.123"
  // https://www.timestamp-converter.com/

  const std::string time_string = "2024-11-17T06:54:12.123";
  const std::string expected_time_string = "2024-11-17T06:54:12.123Z";
  const std::time_t expected_timestamp = 1731826452;
  const std::time_t expected_timestamp_ms = 1731826452123;
  const auto expected_time =
      std::chrono::system_clock::time_point(std::chrono::milliseconds(expected_timestamp_ms));

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  const auto actual_time = fastgpx::parse_gpx_time(time_string);
  CHECK(actual_time == expected_time);

  CHECK(format_iso8601<std::chrono::milliseconds>(actual_time) == expected_time_string);

  const auto actual_timestamp = time_point_to_epoch<std::chrono::milliseconds>(actual_time);
  CHECK(actual_timestamp == expected_timestamp_ms);
}

TEST_CASE("Parse iso8601 invalid string", "[datetime][gpxtime]")
{
  const std::string time_string = "hello";
  REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
}

TEST_CASE("Parse iso8601 invalid string empty", "[datetime][gpxtime]")
{
  const std::string time_string = "";
  REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
}

TEST_CASE("Parse iso8601 invalid string valid length", "[datetime][gpxtime]")
{
  const std::string time_string = "xxxxxxxxxxxxxxxxxxxx";
  REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
}

TEST_CASE("Parse iso8601 invalid year", "[datetime][gpxtime]")
{
  const std::string time_string = "20x4-05-18T07:50:01Z";
  REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
}

TEST_CASE("Parse GPX time outside the representable range", "[datetime][gpxtime]")
{
  // Found by fuzzing (#48). `system_clock::from_time_t` overflows int64 nanoseconds on libstdc++
  // for years outside roughly 1677..2262, and `_mkgmtime` on Windows only covers 1970..3000 and
  // returns -1 otherwise. Both must surface as a parse_error rather than undefined behaviour or
  // a silently wrong value.
  SECTION("far future")
  {
    const std::string time_string = "9999-12-31T23:59:59Z";
    REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
  }
  SECTION("far past")
  {
    const std::string time_string = "0008-07-18T16:07:50.000";
    REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
  }
}

TEST_CASE("Parse GPX time far future within range", "[datetime][gpxtime]")
{
  // 2200-12-31T23:59:59Z fits both a nanosecond system_clock (until 2262) and _mkgmtime (until
  // 3000), so it must parse on every platform.
  const std::string time_string = "2200-12-31T23:59:59Z";
  const std::time_t expected_timestamp = 7289654399;
  const auto expected_time = std::chrono::system_clock::from_time_t(expected_timestamp);

  CAPTURE(time_string, expected_timestamp, expected_time);

  const auto actual_time = fastgpx::parse_gpx_time(time_string);
  CHECK(actual_time == expected_time);
  CHECK(format_iso8601(actual_time) == time_string);
}

TEST_CASE("Parse iso8601 invalid month out of range", "[datetime][gpxtime]")
{
  const std::string time_string = "2024-30-18T07:50:01Z";
  REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
}

TEST_CASE("Parse iso8601 invalid time annotation", "[datetime][gpxtime]")
{
  const std::string time_string = "2024-05-18x07:50:01Z";
  REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
}

TEST_CASE("Parse iso8601 invalid timezone annotation", "[datetime][gpxtime]")
{
  const std::string time_string = "2024-05-18T07:50:01x";
  REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
}

TEST_CASE("Parse iso8601 invalid timezone sign", "[datetime][gpxtime]")
{
  const std::string time_string = "2024-05-18T07:50:01x08:30";
  REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
}

TEST_CASE("Benchmark parse iso8601 date string", "[!benchmark][datetime]")
{
  // The five experimental parsers this was compared against were removed in #47. The recorded
  // numbers are in benchmarks/datetime_parse.md.
  const std::string time_string = "2024-05-18T06:50:01Z";

  BENCHMARK("parse_gpx_time")
  {
    return fastgpx::parse_gpx_time(time_string);
  };
}
