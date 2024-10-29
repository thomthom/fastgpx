#include "fastgpx/datetime.hpp"

namespace fastgpx {

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
  }

  // Time in UTC
  tm.tm_isdst = 0; // Ensure that the parsed time is in UTC (not considering daylight savings)

  const auto time = std::mktime(&tm);
  return std::chrono::system_clock::from_time_t(time);
}

} // namespace v1

} // namespace fastgpx
