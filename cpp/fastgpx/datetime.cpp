#include "fastgpx/datetime.hpp"

#include <ctime>

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

  // Parse ISO 8601 string without fractional seconds.
  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

  // Handle optional fractional seconds if present.
  if (ss.peek() == '.')
  {
    char dot;
    double fractional_seconds;
    ss >> dot >> fractional_seconds;
    // TODO: Can this be handled? Is this data needed?
  }

  // Time in UTC
  // TODO: Is this correct?
  tm.tm_isdst = 0; // Ensure that the parsed time is in UTC (not considering daylight savings)

  // const auto time = std::mktime(&tm);
  const auto time = make_utc_time(&tm);
  return std::chrono::system_clock::from_time_t(time);
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

  std::istringstream ss(time_str);
  std::chrono::system_clock::time_point tp;
  ss >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ", tp);
  return tp;
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
