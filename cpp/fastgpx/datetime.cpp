#include "fastgpx/datetime.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <ctime>
#include <iomanip>
#include <optional>
#include <ranges>
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

namespace v5 {

namespace iso8601 {

enum class TokenType
{
  Unknown,
  Numeric,           // [0-9.]
  Year,              // YYYY
  Month,             // MM
  Week,              // ww
  Day,               // DD
  DayOfYear,         // DDD
  Hour,              // hh
  Minute,            // mm
  Second,            // ss
  Timezone,          // hh:mm hh
  DateSeparator,     // -
  TimeSeparator,     // :
  WeekIndicator,     // W
  TimeIndicator,     // T
  TimezoneIndicator, // Z + -
};

enum class Format
{
  Basic,
  Extended,
};

enum class Parse
{
  Date,
  Time,
  Timezone,
};

struct Token
{
  TokenType type;
  std::string_view value;
  std::optional<std::string_view> fraction;
};

struct Context
{
  Parse parse;
  Format format;
  TokenType last_token_type;
};

} // namespace iso8601

class fastgpx_error : public std::runtime_error
{
public:
  explicit fastgpx_error(const std::string &message) : std::runtime_error(message) {}
};

class parse_error : public fastgpx_error
{
public:
  explicit parse_error(const std::string &message) : fastgpx_error(message) {}
};

std::chrono::system_clock::time_point parse_iso8601(const std::string_view time_str)
{
  // https://en.wikipedia.org/wiki/ISO_8601
  // ISO 8601-1:2019/Amd 1:2022

  // Year
  //
  // YYYY
  // ±YYYYY
  //
  // It therefore represents years from 0000 to 9999, year 0000 being equal to
  // 1 BC and all others AD,
  //
  // To represent years before 0000 or after 9999, the standard also permits the
  // expansion of the year representation but only by prior agreement between the
  // sender and the receiver. An expanded year representation [±YYYYY] must
  // have an agreed-upon number of extra year digits beyond the four-digit minimum,
  // and it must be prefixed with a + or − sign instead of the more common
  // AD/BC (or CE/BCE) notation; by convention 1 BC is labelled +0000, 2 BC is
  // labeled −0001, and so on.

  // Calendar dates
  //
  // Basic format:
  // * YYYYMMDD
  //
  // Extended format:
  // * YYYY-MM-DD
  // * YYYY-MM

  // Week dates
  //
  // Basic format:
  // * YYYYWww
  // * YYYYWwwD
  //
  // Extended format:
  // * YYYY-Www
  // * YYYY-Www-D

  // Ordinal dates
  //
  // Basic format:
  // * YYYYDDD
  //
  // Extended format:
  // * YYYY-DDD

  // Time
  //
  // Basic format:
  // * Thhmmss.sss
  // * Thhmmss
  // * Thhmm.mmm
  // * Thhmm
  //
  // Extended format:
  // * Thh:mm:ss.sss
  // * Thh:mm:ss
  // * Thh:mm.mmm
  // * Thh:mm
  // * Thh.hhh
  // * Thh

  // Time zone designators
  //
  // <time>Z
  // <time>±hh:mm
  // <time>±hhmm
  // <time>±hh
  //
  // Z is optional?
  // * 2024-05-18T07:50:00Z
  // * 2024-05-18T07:50:00

  // Decimals
  //
  // A decimal fraction may be added to the lowest order time element present in
  // any of these representations. A decimal mark, either a comma or a dot on the
  // baseline, is used as a separator between the time element and its fraction.
  // (Following ISO 80000-1 according to ISO 8601:1-2019, it does not
  // stipulate a preference except within International Standards, but with a
  // preference for a comma according to ISO 8601:2004.) For example, to
  // denote "14 hours, 30 and one half minutes", do not include a seconds figure;
  // represent it as "14:30,5", "T1430,5", "14:30.5", or "T1430.5".
  //
  // There is no limit on the number of decimal places for the decimal fraction.
  // However, the number of decimal places needs to be agreed to by the communicating
  // parties. For example, in Microsoft SQL Server, the precision of a decimal
  // fraction is 3 for a DATETIME, i.e., "yyyy-mm-ddThh:mm:ss[.mmm]".

  // Durations / Time intervals
  //
  // Not relevant for GPX parsing.

  // const auto extended_format =
  //     std::ranges::any_of(time_str, [](const char c) { return c == '-' || c == ':'; });
  bool has_date_separator = false;
  bool has_time_separator = false;
  bool has_time = false;
  bool has_timezone = false;
  for (const auto &c : time_str)
  {
    if (c == '-') // Might be timezone indicator.
    {
      has_date_separator = true;
    }
    else if (c == ':')
    {
      has_time_separator = true;
    }
    else if (c == 'T')
    {
      has_time = true;
    }
    else if (c == 'Z')
    {
      has_timezone = true;
    }
  }
  const auto extended_format = has_date_separator || has_time_separator;

  constexpr std::string_view decimals = "0123456789."; // Also comma?

  if (extended_format)
  {
    // constexpr std::string_view separators = "-:TZ";

    auto is_decimal = [](char ch) { return (ch >= '0' && ch <= '9') || ch == '.'; };
    auto is_decimal_chuck = [&is_decimal](char a, char b) {
      return is_decimal(a) == is_decimal(b);
    };

    // Tokenizer

    auto chunks = time_str | std::ranges::views::chunk_by(is_decimal_chuck);
    auto tokens = chunks | std::views::transform([](const auto &chunk) {
                    return iso8601::Token{.value = {chunk.begin(), chunk.end()}};
                  });
    auto data = tokens | std::ranges::to<std::vector>();

    iso8601::Context context;
    for (auto &token : data)
    {
      if (token.value == "T")
      {
        token.type = iso8601::TokenType::TimeSeparator;
        context.parse = iso8601::Parse::Time;
      }

      else if (token.value == "Z")
      {
        token.type = iso8601::TokenType::Timezone;
        context.parse = iso8601::Parse::Timezone;
      }

      else if (token.value == "W")
      {
        token.type = iso8601::TokenType::WeekIndicator;
      }

      else if (token.value == "-")
      {
        if (context.parse == iso8601::Parse::Date)
        {
          token.type = iso8601::TokenType::DateSeparator;
          context.format = iso8601::Format::Extended;
        }
        else if (context.parse == iso8601::Parse::Time)
        {
          // TODO: We probaby don't expect this in Time, but after that parsed.
          // However, we don't know when time might end. It might be hh, hh:mm, hh:mm:ss...
          // TODO: Timezone in format of: +04:00, -04:00, +0400, -0400, ...
          // Since this can be Z, + or -, the + or - need to be applied to the
          // following timezone value.
          token.type = iso8601::TokenType::TimezoneIndicator;
          context.parse = iso8601::Parse::Timezone;
        }
        else
        {
          throw parse_error("Unexpected token: '-'");
        }
      }

      else if (token.value == ":")
      {
        if (context.parse != iso8601::Parse::Time || context.parse != iso8601::Parse::Timezone)
        {
          throw parse_error("Unexpected time separator.");
        }

        token.type = iso8601::TokenType::TimeSeparator;
        context.format = iso8601::Format::Extended;
      }

      else
      {
        if (!std::ranges::all_of(token.value, [](char ch) { return std::isdigit(ch); }))
        {
          throw parse_error("Unexpected non-numeric token.");
        }
        token.type = iso8601::TokenType::Numeric;
        // TODO: context.has_date = true; etc...
      }

      context.last_token_type = token.type;
    }

    // Parser
    // TODO: New context.
    context.parse = iso8601::Parse::Date;
    context.last_token_type == iso8601::TokenType::Unknown;
    if (context.format == iso8601::Format::Basic)
    {
      // Parse Numeric tokens into their components.
      // TODO: ...
    }
    else
    {
      // Parse Extended tokens into their components.
      for (auto &token : data)
      {
        switch (token.type)
        {
        case iso8601::TokenType::Numeric:
        {
          switch (context.parse)
          {
          case iso8601::Parse::Date:
          {
            switch (context.last_token_type)
            {
            case iso8601::TokenType::Unknown:
            {
              if (token.value.size() != 4)
              {
                throw parse_error("Year must be 4 digits.");
              }
              token.type = iso8601::TokenType::Year;
              break;
            }
            case iso8601::TokenType::Year:
            {
              if (token.value.size() == 2)
              {
                token.type = iso8601::TokenType::Month;
              }
              else if (token.value.size() == 3)
              {
                token.type = iso8601::TokenType::DayOfYear;
              }
              throw parse_error("Month must be 2 digits, Day of year must be 3 digits.");
              break;
            }
            break;
            case iso8601::TokenType::WeekIndicator:
            {
              if (token.value.size() != 2)
              {
                throw parse_error("Week must be 2 digits.");
              }
              token.type = iso8601::TokenType::Month;
              break;
            }
            break;
            }
          }
          }
        }

          // Compiler
          std::tm tm = {};
          for (const auto &token : data)
          {
            // TODO: What to do if there is only a time?
            switch (token.type)
            {
            case iso8601::TokenType::Year:
            {
              std::from_chars(token.value.data(), token.value.data() + token.value.size(),
                              tm.tm_year);
              tm.tm_year -= 1900; // Adjust year to be relative to 1900
              break;
            }
            case iso8601::TokenType::Month:
            {
              std::from_chars(token.value.data(), token.value.data() + token.value.size(),
                              tm.tm_mon);
              tm.tm_mon -= 1; // Adjust month to be zero-based
              break;
            }
            case iso8601::TokenType::Day:
            {
              std::from_chars(token.value.data(), token.value.data() + token.value.size(),
                              tm.tm_mday);
              break;
            }
            case iso8601::TokenType::Hour:
            {
              std::from_chars(token.value.data(), token.value.data() + token.value.size(),
                              tm.tm_hour);
              break;
            }
            case iso8601::TokenType::Minute:
            {
              std::from_chars(token.value.data(), token.value.data() + token.value.size(),
                              tm.tm_min);
              break;
            }
            case iso8601::TokenType::Second:
            {
              std::from_chars(token.value.data(), token.value.data() + token.value.size(),
                              tm.tm_sec);
              break;
            }
            case iso8601::TokenType::Timezone:
            {
              // TODO: ...
              break;
            }
            case iso8601::TokenType::DateSeparator:
            case iso8601::TokenType::TimeSeparator:
            case iso8601::TokenType::TimezoneIndicator:
              break; // Skip these tokens.
            default:
              throw parse_error("Unexpected token type.");
            };
          }

          const auto time = make_utc_time(&tm);
          return std::chrono::system_clock::from_time_t(time);
        }
        else
        {
          // Basic format
          // TODO:
        }

        std::tm tm2 = {};
        const auto time2 = make_utc_time(&tm2);
        return std::chrono::system_clock::from_time_t(time2);
      }

    } // namespace v5

  } // namespace fastgpx
