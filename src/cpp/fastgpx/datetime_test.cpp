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

std::string format_iso8601(const std::chrono::utc_clock::time_point& tp)
{
  const auto tp_seconds = std::chrono::floor<std::chrono::seconds>(tp);
  return std::format("{:%Y-%m-%dT%H:%M:%SZ}", tp_seconds);
}

template<typename TIME = std::chrono::seconds>
auto time_point_to_epoch(const std::chrono::system_clock::time_point& tp)
{
  return std::chrono::duration_cast<TIME>(tp.time_since_epoch()).count();
}

auto time_point_to_epoch(const std::chrono::utc_clock::time_point& tp)
{
  // The time_since_epoch isn't returning expected UNIX epoch for utc_clock.
  // For an ISO8601 time/date string representing 1716018601 the utc_clock
  // will return 1716018628.
  const auto sys_tp = std::chrono::utc_clock::to_sys(tp);
  return time_point_to_epoch(sys_tp);
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

  SECTION("v5 std::from_chars parser")
  {
    const auto actual_time = fastgpx::v5::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }

  SECTION("v6 std::from_chars gpx_time")
  {
    const auto actual_time = fastgpx::v6::parse_gpx_time(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
}

TEST_CASE("Parse iso8601 extended time YYYY-MM-DDThh:mmZ", "[datetime]")
{
  // "2024-11-17T06:54Z"
  // https://www.timestamp-converter.com/

  const std::string time_string = "2024-11-17T06:54Z";
  const std::string expected_time_string = "2024-11-17T06:54:00Z";
  const std::time_t expected_timestamp = 1731826440;
  const auto expected_time = std::chrono::system_clock::from_time_t(expected_timestamp);

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  SECTION("v5 std::from_chars parser")
  {
    const auto actual_time = fastgpx::v5::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
}

TEST_CASE("Parse iso8601 extended time YYYY-MM-DDThhZ", "[datetime]")
{
  // "2024-11-17T06Z"
  // https://www.timestamp-converter.com/ <- Note: doesn't handle this format.

  const std::string time_string = "2024-11-17T06Z";
  const std::string expected_time_string = "2024-11-17T06:00:00Z";
  const std::time_t expected_timestamp = 1731823200;
  const auto expected_time = std::chrono::system_clock::from_time_t(expected_timestamp);

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  SECTION("v5 std::from_chars parser")
  {
    const auto actual_time = fastgpx::v5::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
}

TEST_CASE("Parse iso8601 extended date YYYY-MM-DD", "[datetime]")
{
  // "2024-11-17"
  // https://www.timestamp-converter.com/

  const std::string time_string = "2024-11-17";
  const std::string expected_time_string = "2024-11-17T00:00:00Z";
  const std::time_t expected_timestamp = 1731801600;
  const auto expected_time = std::chrono::system_clock::from_time_t(expected_timestamp);

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  SECTION("v5 std::from_chars parser")
  {
    const auto actual_time = fastgpx::v5::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
}

TEST_CASE("Parse iso8601 extended date YYYY-MM", "[datetime]")
{
  // "2024-11"
  // https://www.timestamp-converter.com/

  const std::string time_string = "2024-11";
  const std::string expected_time_string = "2024-11-01T00:00:00Z";
  const std::time_t expected_timestamp = 1730419200;
  const auto expected_time = std::chrono::system_clock::from_time_t(expected_timestamp);

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  SECTION("v5 std::from_chars parser")
  {
    const auto actual_time = fastgpx::v5::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
}

TEST_CASE("Parse iso8601 date YYYY", "[datetime]")
{
  // "2024"
  // https://www.timestamp-converter.com/

  const std::string time_string = "2024";
  const std::string expected_time_string = "2024-01-01T00:00:00Z";
  const std::time_t expected_timestamp = 1704067200;
  const auto expected_time = std::chrono::system_clock::from_time_t(expected_timestamp);

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  SECTION("v5 std::from_chars parser")
  {
    const auto actual_time = fastgpx::v5::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
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

  SECTION("v5 std::from_chars parser")
  {
    const auto actual_time = fastgpx::v5::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601<std::chrono::milliseconds>(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch<std::chrono::milliseconds>(actual_time);
    CHECK(actual_timestamp == expected_timestamp_ms);
  }

  SECTION("v6 std::from_chars gpx_time")
  {
    const auto actual_time = fastgpx::v6::parse_gpx_time(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601<std::chrono::milliseconds>(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch<std::chrono::milliseconds>(actual_time);
    CHECK(actual_timestamp == expected_timestamp_ms);
  }
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

  SECTION("v5 std::from_chars parser")
  {
    const auto actual_time = fastgpx::v5::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }

  SECTION("v6 std::from_chars gpx_time")
  {
    const auto actual_time = fastgpx::v6::parse_gpx_time(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
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

  SECTION("v5 std::from_chars parser")
  {
    const auto actual_time = fastgpx::v5::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }

  SECTION("v6 std::from_chars gpx_time")
  {
    const auto actual_time = fastgpx::v6::parse_gpx_time(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
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

  SECTION("v6 std::from_chars gpx_time")
  {
    const auto actual_time = fastgpx::v6::parse_gpx_time(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601<std::chrono::milliseconds>(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch<std::chrono::milliseconds>(actual_time);
    CHECK(actual_timestamp == expected_timestamp_ms);
  }
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

  SECTION("v6 std::from_chars gpx_time")
  {
    const auto actual_time = fastgpx::v6::parse_gpx_time(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601<std::chrono::milliseconds>(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch<std::chrono::milliseconds>(actual_time);
    CHECK(actual_timestamp == expected_timestamp_ms);
  }
}

TEST_CASE("Parse iso8601 extended date time no timezone", "[datetime]")
{
  // "2024-11-17T06:54:43"
  // https://www.timestamp-converter.com/

  {
    using namespace std::chrono;

    const auto now = std::chrono::system_clock::now();
    const auto* time_zone = std::chrono::current_zone();
    const auto offset = time_zone->get_info(now).offset.count();

    if (offset == 0)
    {
      WARN("System local time is the same as UTC. This test will not be reliable.");
    }
  }

  const std::string time_string = "2024-11-17T06:54:43";

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
  std::istringstream ss(time_string);
  std::chrono::sys_time<std::chrono::seconds> expected_time;
  ss >> std::chrono::parse("%Y-%m-%dT%H:%M:%S", expected_time);

  const std::string expected_time_string = "2024-11-17T06:54:43Z";
  const auto expected_timestamp = expected_time.time_since_epoch().count();

  CAPTURE(time_string, expected_time_string);

  SECTION("v5 std::from_chars parser")
  {
    const auto actual_time = fastgpx::v5::parse_iso8601(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
}

TEST_CASE("Parse iso8601 extended time Thh:mm::ss", "[datetime][invalid]")
{
  const std::string time_string = "T15:24:43Z";
  SECTION("v5 std::from_chars parser")
  {
    // Only time make no sense in context of GPX timestamps.
    REQUIRE_THROWS_AS(fastgpx::v5::parse_iso8601(time_string), fastgpx::parse_error);
  }
}

TEST_CASE("Parse GPX time missing timezone YYYY-MM-DDThh:mm:ss", "[datetime][gpxtime]")
{
  // "2024-11-17T06:54:12"
  // https://www.timestamp-converter.com/

  const std::string time_string = "2024-11-17T06:54:12";
  const std::string expected_time_string = "2024-11-17T06:54:12Z";
  const std::time_t expected_timestamp = 1731826452;
  const auto expected_time = std::chrono::system_clock::from_time_t(expected_timestamp);

  CAPTURE(time_string, expected_timestamp, expected_time_string, expected_time);

  SECTION("v6 std::from_chars gpx_time")
  {
    const auto actual_time = fastgpx::v6::parse_gpx_time(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch(actual_time);
    CHECK(actual_timestamp == expected_timestamp);
  }
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

  SECTION("v6 std::from_chars gpx_time")
  {
    const auto actual_time = fastgpx::v6::parse_gpx_time(time_string);
    CHECK(actual_time == expected_time);

    CHECK(format_iso8601<std::chrono::milliseconds>(actual_time) == expected_time_string);

    const auto actual_timestamp = time_point_to_epoch<std::chrono::milliseconds>(actual_time);
    CHECK(actual_timestamp == expected_timestamp_ms);
  }
}

TEST_CASE("Parse iso8601 extended time YYYY-DDDThh:mm:ssZ", "[datetime]")
{
  // https://dencode.com/en/date/iso8601
  //
  // ISO8601 Date               20240517T095001+0200
  // ISO8601 Date (Extend)      2024-05-17T09:50:01+02:00
  // ISO8601 Date (Week)        2024-W20-5T09:50:01+02:00
  // ISO8601 Date (Ordinal)     2024-138T09:50:01+02:00

  // https://www.timestamp-converter.com/
  //
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

TEST_CASE("Parse iso8601 invalid string", "[datetime][gpxtime]")
{
  const std::string time_string = "hello";
  SECTION("v6 std::from_chars gpx_time")
  {
    REQUIRE_THROWS_AS(fastgpx::v6::parse_gpx_time(time_string), fastgpx::parse_error);
  }
}

TEST_CASE("Parse iso8601 invalid string empty", "[datetime][gpxtime]")
{
  const std::string time_string = "";
  SECTION("v6 std::from_chars gpx_time")
  {
    REQUIRE_THROWS_AS(fastgpx::v6::parse_gpx_time(time_string), fastgpx::parse_error);
  }
}

TEST_CASE("Parse iso8601 invalid string valid length", "[datetime][gpxtime]")
{
  const std::string time_string = "xxxxxxxxxxxxxxxxxxxx";
  SECTION("v6 std::from_chars gpx_time")
  {
    REQUIRE_THROWS_AS(fastgpx::v6::parse_gpx_time(time_string), fastgpx::parse_error);
  }
}

TEST_CASE("Parse iso8601 invalid year", "[datetime][gpxtime]")
{
  const std::string time_string = "20x4-05-18T07:50:01Z";
  SECTION("v6 std::from_chars gpx_time")
  {
    REQUIRE_THROWS_AS(fastgpx::v6::parse_gpx_time(time_string), fastgpx::parse_error);
  }
}

TEST_CASE("Parse iso8601 invalid month out of range", "[datetime][gpxtime]")
{
  const std::string time_string = "2024-30-18T07:50:01Z";
  SECTION("v6 std::from_chars gpx_time")
  {
    REQUIRE_THROWS_AS(fastgpx::v6::parse_gpx_time(time_string), fastgpx::parse_error);
  }
}

TEST_CASE("Parse iso8601 invalid time annotation", "[datetime][gpxtime]")
{
  const std::string time_string = "2024-05-18x07:50:01Z";
  SECTION("v6 std::from_chars gpx_time")
  {
    REQUIRE_THROWS_AS(fastgpx::v6::parse_gpx_time(time_string), fastgpx::parse_error);
  }
}

TEST_CASE("Parse iso8601 invalid timezone annotation", "[datetime][gpxtime]")
{
  const std::string time_string = "2024-05-18T07:50:01x";
  SECTION("v6 std::from_chars gpx_time")
  {
    REQUIRE_THROWS_AS(fastgpx::v6::parse_gpx_time(time_string), fastgpx::parse_error);
  }
}

TEST_CASE("Parse iso8601 invalid timezone sign", "[datetime][gpxtime]")
{
  const std::string time_string = "2024-05-18T07:50:01x08:30";
  SECTION("v6 std::from_chars gpx_time")
  {
    REQUIRE_THROWS_AS(fastgpx::v6::parse_gpx_time(time_string), fastgpx::parse_error);
  }
}

TEST_CASE("Benchmark parse iso8601 date string", "[!benchmark][datetime]")
{
  const std::string time_string = "2024-05-18T06:50:01Z";

  BENCHMARK("v1 std::get_time")
  {
    return fastgpx::v1::parse_iso8601(time_string);
  };
  BENCHMARK("v2 std::chrono::parse")
  {
    return fastgpx::v2::parse_iso8601(time_string);
  };
  BENCHMARK("v3 std::chrono::parse")
  {
    return fastgpx::v3::parse_iso8601(time_string);
  };
  BENCHMARK("v4 std::from_chars")
  {
    return fastgpx::v4::parse_iso8601(time_string);
  };
  BENCHMARK("v5 std::from_chars parser")
  {
    return fastgpx::v5::parse_iso8601(time_string);
  };
  BENCHMARK("v6 std::from_chars gpx_time")
  {
    return fastgpx::v6::parse_gpx_time(time_string);
  };
}
