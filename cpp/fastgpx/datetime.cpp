#include "fastgpx/datetime.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>
#include <ctime>
#include <iomanip>
#include <optional>
#include <ranges>
#include <sstream>
#include <unordered_map>

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

enum class ChunkType
{
  Unknown,
  Decimal,           // [0-9]
  DecimalSeparator,  // .
  Hyphen,            // -
  Plus,              // +
  WeekIndicator,     // W
  TimeIndicator,     // T
  TimeSeparator,     // :
  TimezoneIndicator, // Z
};

struct Chunk
{
  ChunkType type;
  std::string_view data;
};

enum class TokenType
{
  Unknown,
  Year,              // YYYY
  Month,             // MM
  WeekIndicator,     // W
  Week,              // ww
  Day,               // DD
  DayOfYear,         // DDD
  Hour,              // hh
  Minute,            // mm
  Second,            // ss
  DateSeparator,     // -
  TimeIndicator,     // T
  TimeSeparator,     // :
  TimezoneIndicator, // Z
  TimezonePositive,  // +
  TimezoneNegative,  // -
  TimezoneHour,      // hh
  TimezoneMinute,    // mm
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
  Done,
};

struct Decimal
{
  int integral;
  std::optional<int> fractional;
};

struct Token
{
  TokenType type;
  std::optional<Decimal> decimal;
};

struct Context
{
  Parse parse;
  Format format;
  ChunkType last_chunk_type;
  TokenType last_decimal_token_type;
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

namespace {

iso8601::TokenType parse_date_type(const iso8601::Chunk &chunk, const iso8601::Context &context)
{
  assert(context.parse == iso8601::Parse::Date);
  assert(chunk.type == iso8601::ChunkType::Decimal);

  if (context.last_chunk_type == iso8601::ChunkType::WeekIndicator)
  {
    return iso8601::TokenType::Week;
  }

  switch (context.last_decimal_token_type)
  {
  case iso8601::TokenType::Unknown:
    return iso8601::TokenType::Year;
  case iso8601::TokenType::Year:
    if (chunk.data.size() == 3) // TODO: Validate chunk sizes.
    {
      return iso8601::TokenType::DayOfYear;
    }
    return iso8601::TokenType::Month;
  case iso8601::TokenType::Month:
    return iso8601::TokenType::Day;
  default:
    throw parse_error("unexpected date decimal");
  }
}

iso8601::TokenType parse_time_type(const iso8601::Chunk &chunk, const iso8601::Context &context)
{
  assert(context.parse == iso8601::Parse::Time);
  assert(chunk.type == iso8601::ChunkType::Decimal);

  if (context.last_chunk_type == iso8601::ChunkType::TimeIndicator)
  {
    return iso8601::TokenType::Hour;
  }

  switch (context.last_decimal_token_type)
  {
  case iso8601::TokenType::Hour:
    return iso8601::TokenType::Minute;
  case iso8601::TokenType::Minute:
    return iso8601::TokenType::Second;
  default:
    throw parse_error("unexpected time decimal");
  }
}

iso8601::TokenType parse_timezone_type(const iso8601::Chunk &chunk, const iso8601::Context &context)
{
  assert(context.parse == iso8601::Parse::Timezone);
  assert(chunk.type == iso8601::ChunkType::Decimal);

  switch (context.last_decimal_token_type)
  {
  case iso8601::TokenType::Hour:
  case iso8601::TokenType::Minute:
  case iso8601::TokenType::Second:
    return iso8601::TokenType::TimezoneHour;
  case iso8601::TokenType::TimezoneHour:
    return iso8601::TokenType::TimezoneMinute;
  default:
    throw parse_error("unexpected timezone decimal");
  }
}

iso8601::Token parse_decimal(const iso8601::Chunk &chunk, const iso8601::Context &context)
{
  iso8601::Token token;
  if (context.parse == iso8601::Parse::Date)
  {
    token.type = parse_date_type(chunk, context);
  }
  else if (context.parse == iso8601::Parse::Time)
  {
    token.type = parse_time_type(chunk, context);
  }
  else if (context.parse == iso8601::Parse::Timezone)
  {
    token.type = parse_timezone_type(chunk, context);
  }

  int value;
  std::from_chars(chunk.data.data(), chunk.data.data() + chunk.data.size(), value);
  token.decimal = iso8601::Decimal{.integral = value};
  return token;
}

} // namespace

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

  const std::unordered_map<char, iso8601::ChunkType> char_to_chunk_type{
      {'-', iso8601::ChunkType::Hyphen},            //
      {'+', iso8601::ChunkType::Plus},              //
      {'.', iso8601::ChunkType::DecimalSeparator},  //
      {'T', iso8601::ChunkType::TimeIndicator},     //
      {':', iso8601::ChunkType::TimeSeparator},     //
      {'Z', iso8601::ChunkType::TimezoneIndicator}, //
      {'W', iso8601::ChunkType::WeekIndicator},     //
  };

  auto is_digit_chuck = [](char a, char b) { return std::isdigit(a) == std::isdigit(b); };

  auto chuck_type = [&char_to_chunk_type](const auto &data) {
    const auto ch = *data.begin();
    if (std::isdigit(ch))
    {
      return iso8601::ChunkType::Decimal;
    }
    else
    {
      try
      {
        return char_to_chunk_type.at(ch);
      }
      catch (const std::out_of_range &error)
      {
        throw parse_error("unexpected chunk data"); // TODO: include ch
      };
    }
  };

  auto make_chunk = [&chuck_type](const auto &data) {
    return iso8601::Chunk{.type = chuck_type(data), .data = {data.begin(), data.end()}};
  };

  auto chunks = time_str | std::views::chunk_by(is_digit_chuck) | std::views::transform(make_chunk);
  const auto num_chunks = std::ranges::distance(chunks);
  auto data = chunks | std::ranges::to<std::vector>(); // TODO: Debug. Remove.

  iso8601::Context context{};
  std::vector<iso8601::Token> tokens;
  for (const auto &chunk : chunks)
  {
    if (tokens.empty() && chunk.type != iso8601::ChunkType::Decimal)
    {
      throw parse_error("Unexpected chunk type.");
    }

    switch (chunk.type)
    {
    case iso8601::ChunkType::Decimal:
    {
      if (context.last_chunk_type == iso8601::ChunkType::DecimalSeparator)
      {
        if (tokens.empty() || !tokens.back().decimal.has_value())
        {
          throw parse_error("Unexpected fractional.");
        }
        int value;
        std::from_chars(chunk.data.data(), chunk.data.data() + chunk.data.size(), value);
        tokens.back().decimal->fractional = value;
        if (context.parse == iso8601::Parse::Date)
        {
          context.parse = iso8601::Parse::Time;
        }
        else if (context.parse == iso8601::Parse::Time)
        {
          context.parse = iso8601::Parse::Timezone;
        }
        else if (context.parse == iso8601::Parse::Timezone)
        {
          context.parse = iso8601::Parse::Done;
        }
      }
      const auto token = parse_decimal(chunk, context);

      tokens.emplace_back(token);
      context.last_decimal_token_type = token.type;
      break;
    }
    case iso8601::ChunkType::DecimalSeparator:
    {
      if (context.last_chunk_type != iso8601::ChunkType::DecimalSeparator)
      {
        throw parse_error("Unexpected decimal separator.");
      }
      // TODO: ...
      break;
    }
    case iso8601::ChunkType::Hyphen:
    {
      switch (context.parse)
      {
      case iso8601::Parse::Date:
      {
        const auto &token =
            tokens.emplace_back(iso8601::Token{.type = iso8601::TokenType::DateSeparator});
        break;
      }
      case iso8601::Parse::Time:
      {
        const auto &token =
            tokens.emplace_back(iso8601::Token{.type = iso8601::TokenType::TimezoneNegative});
        context.parse = iso8601::Parse::Timezone;
        break;
      }
      default:
      {
        throw parse_error("Unexpected token: -");
      }
      }
      break;
    case iso8601::ChunkType::Plus:
    {
      if (context.parse != iso8601::Parse::Time)
      {
        throw parse_error("Unexpected token: +");
      }
      const auto &token =
          tokens.emplace_back(iso8601::Token{.type = iso8601::TokenType::TimezonePositive});
      context.parse = iso8601::Parse::Timezone;
      break;
    }
    case iso8601::ChunkType::TimeSeparator:
    {
      if (context.parse != iso8601::Parse::Time)
      {
        throw parse_error("Unexpected time separator.");
      }
      const auto &token =
          tokens.emplace_back(iso8601::Token{.type = iso8601::TokenType::TimeSeparator});
      break;
    }
    case iso8601::ChunkType::TimeIndicator:
    {
      const auto &token =
          tokens.emplace_back(iso8601::Token{.type = iso8601::TokenType::TimeIndicator});
      context.parse = iso8601::Parse::Time;
      break;
    }
    case iso8601::ChunkType::TimezoneIndicator:
    {
      const auto &token =
          tokens.emplace_back(iso8601::Token{.type = iso8601::TokenType::TimezoneIndicator});
      context.parse = iso8601::Parse::Done;
      break;
    }
    }
    }
    context.last_chunk_type = chunk.type;
  }

  std::tm tm = {};
  for (const auto &token : tokens)
  {
    // TODO: Handle fractional.
    switch (token.type)
    {
    case iso8601::TokenType::Year:
      // Adjust year to be relative to 1900
      tm.tm_year = token.decimal->integral - 1900;
      break;
    case iso8601::TokenType::Month:
      // Adjust month to be zero-based
      tm.tm_mon = token.decimal->integral - 1;
      break;
    case iso8601::TokenType::Day:
      tm.tm_mday = token.decimal->integral;
      break;
    case iso8601::TokenType::Week:
      // TODO: ...
      break;
    case iso8601::TokenType::DayOfYear:
      // TODO: ...
      break;
    case iso8601::TokenType::Hour:
      tm.tm_hour = token.decimal->integral;
      break;
    case iso8601::TokenType::Minute:
      tm.tm_min = token.decimal->integral;
      break;
    case iso8601::TokenType::Second:
      tm.tm_sec = token.decimal->integral;
      break;
    case iso8601::TokenType::TimezoneHour:
    case iso8601::TokenType::TimezoneMinute:
      // TODO: ...
      break;
    }
  }
  const auto time = make_utc_time(&tm);
  return std::chrono::system_clock::from_time_t(time);
}

} // namespace v5

} // namespace fastgpx
