#include <chrono>
#include <format>
#include <ratio>
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

TEST_CASE("Parse GPX time with any number of fractional digits", "[datetime][gpxtime]")
{
  // xsd:dateTime does not limit the fractional digits. One digit and seven digits both occur in
  // gpxpy's test corpus (unicode_with_bom.gpx and Mojstrovka.gpx); .NET's round-trip format
  // writes seven. Digits beyond nanoseconds are dropped, as are those beyond the clock's
  // resolution.
  using namespace std::chrono;
  const auto base = system_clock::from_time_t(1731826452); // 2024-11-17T06:54:12Z

  const auto [suffix, fraction] = GENERATE(table<std::string, nanoseconds>({
      {".5Z", 500ms},
      {".12Z", 120ms},
      {".123Z", 123ms},
      {".1234Z", 123400us},
      {".123456Z", 123456us},
      {".1234567Z", 123456700ns},
      {".123456789Z", 123456789ns},
      {".1234567891234Z", 123456789ns},
      {",5Z", 500ms},
      {".5", 500ms},
      {".000000Z", 0ns},
  }));
  const std::string time_string = "2024-11-17T06:54:12" + suffix;
  CAPTURE(time_string);

  const auto expected = base + duration_cast<system_clock::duration>(fraction);
  CHECK(fastgpx::parse_gpx_time(time_string) == expected);
}

TEST_CASE("Parse GPX time with fractional digits and a timezone offset", "[datetime][gpxtime]")
{
  using namespace std::chrono;
  // 2024-11-17T06:54:12.5+01:30 is 2024-11-17T05:24:12.5Z
  const auto expected =
      system_clock::from_time_t(1731821052) + duration_cast<system_clock::duration>(500ms);
  CHECK(fastgpx::parse_gpx_time("2024-11-17T06:54:12.5+01:30") == expected);
  CHECK(fastgpx::parse_gpx_time("2024-11-17T06:54:12.500000+01:30") == expected);

  // Digits below the clock's resolution are dropped regardless of the offset sign. With the
  // fraction and offset truncated together, a positive offset would round this up to the next
  // second on a 100 ns or microsecond clock.
  const auto zulu = fastgpx::parse_gpx_time("2024-11-17T05:24:12.999999999Z");
  CHECK(fastgpx::parse_gpx_time("2024-11-17T06:54:12.999999999+01:30") == zulu);
  CHECK(fastgpx::parse_gpx_time("2024-11-17T03:54:12.999999999-01:30") == zulu);
  CHECK(zulu < system_clock::from_time_t(1731821053));
}

TEST_CASE("Parse GPX time with malformed fraction or trailing characters", "[datetime][gpxtime]")
{
  const auto time_string =
      GENERATE("2024-11-17T06:54:12.", "2024-11-17T06:54:12.Z", "2024-11-17T06:54:12.5X",
               "2024-11-17T06:54:12.5Zx", "2024-11-17T06:54:12.5+01:00x", "2024-11-17T06:54:12Zx",
               "2024-11-17T06:54:12.5.5Z", "2024-11-17T06:54:12 ");
  CAPTURE(time_string);
  REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
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

// A nanosecond `system_clock` (libstdc++) reaches from roughly 1677 to 2262, which is narrower
// than the four digit years `parse_gpx_time` accepts. Coarser ticks (100 ns on MSVC, microseconds
// on libc++) cover every one of them, and there the four digit year is the only limit.
constexpr bool nanosecond_system_clock =
    std::ratio_less_equal_v<std::chrono::system_clock::period, std::nano>;

TEST_CASE("Parse GPX time outside the representable range", "[datetime][gpxtime]")
{
  // Found by fuzzing (#48). Converting the seconds since the epoch to `system_clock::duration`
  // overflows int64 nanoseconds on libstdc++ for years outside roughly 1677..2262, which is
  // undefined behaviour; it has to surface as a parse_error instead. On a clock with coarser
  // ticks the same dates are representable and have to parse.
  SECTION("far future")
  {
    const std::string time_string = "9999-12-31T23:59:59Z";
    if constexpr (nanosecond_system_clock)
    {
      REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
    }
    else
    {
      CHECK(format_iso8601(fastgpx::parse_gpx_time(time_string)) == time_string);
    }
  }
  SECTION("far past")
  {
    const std::string time_string = "0008-07-18T16:07:50.000";
    if constexpr (nanosecond_system_clock)
    {
      REQUIRE_THROWS_AS(fastgpx::parse_gpx_time(time_string), fastgpx::parse_error);
    }
    else
    {
      CHECK(format_iso8601(fastgpx::parse_gpx_time(time_string)) == "0008-07-18T16:07:50Z");
    }
  }
}

TEST_CASE("Parse GPX time before the Unix epoch", "[datetime][gpxtime]")
{
  // `_mkgmtime` rejected everything before 1970, so this used to throw on Windows and parse
  // everywhere else. The civil arithmetic that replaced it has no such limit. See #19.
  const std::string time_string = "1960-06-15T12:00:00Z";
  const auto actual_time = fastgpx::parse_gpx_time(time_string);
  CHECK(time_point_to_epoch(actual_time) == -301233600);
  CHECK(format_iso8601(actual_time) == time_string);
}

TEST_CASE("Parse GPX time normalises fields past the end of their range", "[datetime][gpxtime]")
{
  // Hour 24, second 60 and a day past the end of the month all carry into the next unit, the
  // same way `timegm` normalised them.
  CHECK(format_iso8601(fastgpx::parse_gpx_time("2024-05-18T24:00:00Z")) == "2024-05-19T00:00:00Z");
  CHECK(format_iso8601(fastgpx::parse_gpx_time("2024-06-30T23:59:60Z")) == "2024-07-01T00:00:00Z");
  CHECK(format_iso8601(fastgpx::parse_gpx_time("2024-02-31T10:00:00Z")) == "2024-03-02T10:00:00Z");
  CHECK(format_iso8601(fastgpx::parse_gpx_time("2023-02-31T10:00:00Z")) == "2023-03-03T10:00:00Z");
}

TEST_CASE("Parse GPX time outside a four digit year", "[datetime][gpxtime]")
{
  // The year in the string is four digits, and `datetime.datetime` in the Python bindings only
  // holds years 1 through 9999. Year 0 and a timezone offset that carries either end past the
  // boundary have to be rejected on every platform, not only on the ones whose `system_clock`
  // happens to be too narrow for them. See #19.
  //
  // These are the same parse_error either way, so on a nanosecond `system_clock` the assertions
  // hold through the narrower `system_clock` check instead; the four digit year check is only
  // reachable on a clock that reaches further than year 9999.
  CHECK_THROWS_AS(fastgpx::parse_gpx_time("0000-01-01T00:00:00Z"), fastgpx::parse_error);
  CHECK_THROWS_AS(fastgpx::parse_gpx_time("0000-12-31T23:59:59Z"), fastgpx::parse_error);
  CHECK_THROWS_AS(fastgpx::parse_gpx_time("0001-01-01T00:00:00+01:00"), fastgpx::parse_error);
  CHECK_THROWS_AS(fastgpx::parse_gpx_time("9999-12-31T23:59:59-01:00"), fastgpx::parse_error);
}

TEST_CASE("Parse GPX time far future within range", "[datetime][gpxtime]")
{
  // 2200-12-31T23:59:59Z fits a nanosecond system_clock, which reaches until 2262, so it must
  // parse on every platform.
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
