#include "fastgpx/datetime.hpp"

#include <charconv>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fastgpx {
namespace {

time_t make_utc_time(std::tm *tm)
{
#ifdef _WIN32
  return _mkgmtime(tm);
#else
  return timegm(tm);
#endif
}

} // namespace

namespace v1 {

std::chrono::system_clock::time_point parse_iso8601(const std::string &time_str)
{
  std::tm tm = {};
  std::istringstream ss(time_str);

  // Basic date-time format (YYYY-MM-DDTHH:MM:SSZ)
  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

  // Basic format without separators (YYYYMMDDTHHMMSSZ)
  if (ss.fail())
  {
    ss.clear();  // Clear the error state of the stream
    ss.seekg(0); // Reset to the beginning of the stream
    ss >> std::get_time(&tm, "%Y%m%dT%H%M%S");
  }

  // Ordinal dates (YYYY-DDDTHH:MM:SSZ)
  if (ss.fail())
  {
    ss.clear();  // Clear the error state of the stream
    ss.seekg(0); // Reset to the beginning of the stream
    ss >> std::get_time(&tm, "%Y-%jT%H:%M:%S");
  }

  if (ss.fail())
  {
    throw std::runtime_error("Failed to parse date-time components.");
  }

  // Initialize the time_point with seconds since epoch.
  const time_t time = make_utc_time(&tm);
  auto time_point = std::chrono::system_clock::from_time_t(time);

  // Handle optional fractional seconds if present.
  if (ss.peek() == '.')
  {
    char dot;
    double fractional_seconds = 0.0;

    // Extract the '.' and the fractional seconds value.
    ss >> dot >> fractional_seconds;

    const auto duration = std::chrono::duration<double, std::milli>(fractional_seconds);
    const auto millis = std::chrono::duration_cast<std::chrono::system_clock::duration>(duration);

    time_point += millis;
  }

  return time_point;
}

} // namespace v1

namespace v2 {

std::chrono::utc_clock::time_point parse_iso8601(const std::string &time_str)
{
  // https://github.com/pybind/pybind11/discussions/3451

  std::istringstream ss(time_str);
  std::chrono::utc_clock::time_point tp;
  ss >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ", tp);
  return tp;
}

} // namespace v2

namespace v3 {

std::chrono::system_clock::time_point parse_iso8601(const std::string &time_str)
{
  // https://github.com/pybind/pybind11/discussions/3451

  // https://stackoverflow.com/questions/26895428/how-do-i-parse-an-iso-8601-date-with-optional-milliseconds-to-a-struct-tm-in-c

  std::istringstream ss(time_str);
  std::chrono::sys_time<std::chrono::milliseconds> tp;

  // Basic date-time format (YYYY-MM-DDTHH:MM:SSZ)
  ss >> std::chrono::parse("%FT%TZ", tp);

  // Basic format with milliseconds in extended format (e.g., 2024-05-18T07:50:01.123Z)
  if (ss.fail())
  {
    ss.clear();
    ss.seekg(0);
    ss >> std::chrono::parse("%FT%T%EfZ", tp);
  }

  // Including microseconds (YYYY-MM-DDTHH:MM:SS.ssssssZ)
  if (ss.fail())
  {
    ss.clear();
    ss.seekg(0);
    // Year/Month/Day:
    // %F:  Equivalent to "%Y-%m-%d". If the width is specified, it is only
    //      applied to the %Y.
    //
    // Hour/Minute/Second:
    // %T:  Equivalent to "%H:%M:%S".
    //
    // Timezone:
    // %z:  Parses the offset from UTC in the format [+|-]hh[mm].
    //      For example -0430 refers to 4 hours 30 minutes behind UTC and 04
    //      refers to 4 hours ahead of UTC.
    //
    // %Ez: The modified commands %Ez and %Oz parses the format [+|-]h[h][:mm]
    //      (i.e., requiring a : between the hours and minutes and making the
    //      leading zero for hour optional).
    ss >> std::chrono::parse("%FT%T%Ez", tp);
  }

  // Basic format without separators (YYYYMMDDTHHMMSSZ)
  if (ss.fail())
  {
    ss.clear();
    ss.seekg(0);
    ss >> std::chrono::parse("%Y%m%dT%H%M%SZ", tp);
  }

  // Compact format with fractional seconds (YYYYMMDDTHHMMSS.FFFZ)
  if (ss.fail())
  {
    ss.clear();
    ss.seekg(0);
    ss >> std::chrono::parse("%Y%m%dT%H%M%S%EfZ", tp);
  }

  // Ordinal dates (YYYY-DDDTHH:MM:SSZ)
  if (ss.fail())
  {
    ss.clear();
    ss.seekg(0);
    ss >> std::chrono::parse("%Y-%jT%T%Z", tp);
  }

  // Week dates (YYYY-Www-DTHH:MM:SSZ)
  if (ss.fail())
  {
    ss.clear();
    ss.seekg(0);
    ss >> std::chrono::parse("%G-W%V-%uT%T%Z", tp);
  }

  if (ss.fail())
  {
    throw std::runtime_error("Failed to parse date-time components.");
  }

  return tp;

  // std::istringstream ss(time_str);
  // std::chrono::system_clock::time_point tp;
  // ss >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ", tp);
  // return tp;
}

} // namespace v3

namespace v4 {

std::chrono::system_clock::time_point parse_iso8601(const std::string_view time_str)
{
  std::tm tm = {};

  // Assuming string: 2024-05-18T07:50:01Z

  // 2024-05-18T07:50:01Z
  // ^^^^
  const auto year_str = time_str.substr(0, 4);
  std::from_chars(year_str.data(), year_str.data() + year_str.size(), tm.tm_year);
  tm.tm_year -= 1900; // Adjust year to be relative to 1900

  // 2024-05-18T07:50:01Z
  //      ^^
  const auto month_str = time_str.substr(5, 2);
  std::from_chars(month_str.data(), month_str.data() + month_str.size(), tm.tm_mon);
  tm.tm_mon -= 1; // Adjust month to be zero-based

  // 2024-05-18T07:50:01Z
  //         ^^
  const auto day_str = time_str.substr(8, 2);
  std::from_chars(day_str.data(), day_str.data() + day_str.size(), tm.tm_mday);

  // 2024-05-18T07:50:01Z
  //            ^^
  const auto hour_str = time_str.substr(11, 2);
  std::from_chars(hour_str.data(), hour_str.data() + hour_str.size(), tm.tm_hour);

  // 2024-05-18T07:50:01Z
  //               ^^
  const auto minute_str = time_str.substr(14, 2);
  std::from_chars(minute_str.data(), minute_str.data() + minute_str.size(), tm.tm_min);

  // 2024-05-18T07:50:01Z
  //                  ^^
  const auto second_str = time_str.substr(17, 2);
  std::from_chars(second_str.data(), second_str.data() + second_str.size(), tm.tm_sec);

  // const auto time = std::mktime(&tm);
  const auto time = make_utc_time(&tm);
  return std::chrono::system_clock::from_time_t(time);
}

} // namespace v4

} // namespace fastgpx
