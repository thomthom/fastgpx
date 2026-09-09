#include "fastgpx/datetime.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <format>
#include <iterator>
#include <string_view>
#include <system_error>
#include <type_traits>

#include "fastgpx/errors.hpp"

namespace fastgpx {
namespace {

time_t make_utc_time(std::tm* tm)
{
#ifdef _WIN32
  return _mkgmtime(tm);
#else
  return timegm(tm);
#endif
}

// Converts broken-down UTC time plus a sub-second/timezone adjustment to a `system_clock`
// time point, rejecting anything the platform cannot represent. Found by fuzzing (#48):
//
// - `system_clock::from_time_t` converts seconds to `system_clock::duration`. On libstdc++ that
//   is nanoseconds, so dates outside roughly 1677-09-21..2262-04-11 overflow `int64_t`, which is
//   undefined behaviour. MSVC uses 100 ns ticks and covers a far wider range.
// - `_mkgmtime` (Windows) only supports 1970-01-01..3000-12-31 and returns -1 with `errno` set
//   for anything else. Without the check the caller would silently get 1969-12-31T23:59:59Z.
//   glibc's `timegm` sets `errno` to `EOVERFLOW` for values it cannot represent.
std::chrono::system_clock::time_point to_system_clock_time(std::tm* tm,
                                                           std::chrono::milliseconds adjustment,
                                                           std::string_view time_str)
{
  using namespace std::chrono;
  using sys_duration = system_clock::duration;

  errno = 0;
  const time_t time = make_utc_time(tm);
  if (time == time_t{-1} && errno != 0)
  {
    throw parse_error(std::format("time cannot be represented on this platform: \"{}\"", time_str));
  }

  // Leave room for the adjustment, which is at most one day plus a fraction of a second.
  constexpr auto margin = days(2);
  constexpr auto max_seconds = floor<seconds>(sys_duration::max()) - margin;
  constexpr auto min_seconds = ceil<seconds>(sys_duration::min()) + margin;
  const auto since_epoch = seconds(static_cast<std::int64_t>(time));
  if (since_epoch > max_seconds || since_epoch < min_seconds)
  {
    throw parse_error(std::format("time is outside the range of system_clock: \"{}\"", time_str));
  }

  return system_clock::time_point(duration_cast<sys_duration>(since_epoch)) + adjustment;
}

enum Iso8601FormatLength
{
  DateTimeZulu = 20,                   // YYYY-MM-DDThh:mm:ssZ
  DateTimeMilliSecondsZulu = 24,       // YYYY-MM-DDThh:mm:ss.sssZ
  DateTimeTimeZone = 25,               // YYYY-MM-DDThh:mm:ss±hh:mm
  DateTimeMilliSecondsTimeZone = 29,   // YYYY-MM-DDThh:mm:ss.sss±hh:mm
  DateTimeNoTimezone = 19,             // YYYY-MM-DDThh:mm:ss
  DateTimeMilliSecondsNoTimezone = 23, // YYYY-MM-DDThh:mm:ss.sss
};

template<typename T>
class ParsedValue
{
public:
  explicit ParsedValue(const T& value) : value_(value) {};

  T value() const { return value_; }

  template<typename... Args>
  ParsedValue<T>& OneOf(Args... args)
  {
    static_assert((std::is_same_v<T, Args> && ...), "All arguments must have the same type as T");

    if (!((value_ == args) || ...))
    {
      const auto message = std::format("unexpected value: {}", value_);
      throw parse_error(message);
    }
    return *this;
  }

  // Inclusive range: [min, max]
  ParsedValue<T>& InRange(const T& min, const T& max)
  {
    if (value_ < min or value_ > max)
    {
      const auto message =
          std::format("value out or range: {} (expected: [{}, {}])", value_, min, max);
      throw parse_error(message);
    }
    return *this;
  }

private:
  T value_;
};

class StringParser
{
public:
  StringParser(std::string_view input) : input_(input), it_(input.begin()) {}

  ParsedValue<int> ExtractInt(size_t num)
  {
    const auto remaining = static_cast<size_t>(std::distance(it_, std::end(input_)));
    if (remaining < num)
    {
      throw parse_error("not enough characters to extract", input_, offset(), remaining);
    }
    const auto it_end = std::next(it_, static_cast<std::ptrdiff_t>(num));
    const std::string_view view(it_, it_end);
    if (!std::ranges::all_of(view, [](unsigned char ch) { return std::isdigit(ch) != 0; }))
    {
      throw parse_error("characters are not all digits", input_, offset(), num);
    }

    const auto start = &(*it_);
    const auto end = start + num;
    int out;
    const auto [_ptr, ec] = std::from_chars(start, end, out);
    if (ec == std::errc::invalid_argument || ec == std::errc::result_out_of_range)
    {
      throw parse_error("unable to extract numeric value", input_, offset(), num);
    }
    std::advance(it_, num);
    return ParsedValue<int>(out);
  }

  ParsedValue<char> ExtractChar()
  {
    if (it_ == std::end(input_))
    {
      throw parse_error("unable to extract character after end of string");
    }
    char out = *it_;
    std::advance(it_, 1);
    return ParsedValue<char>(out);
  }

  void Expect(char ch)
  {
    if (it_ == std::end(input_))
    {
      const auto message = std::format("unexpected end of string (expected: {})", ch);
      throw parse_error(message);
    }
    if (*it_ != ch)
    {
      const auto message = std::format("unexpected character: {} (expected: {})", *it_, ch);
      throw parse_error(message, input_, offset(), 1);
    }
    std::advance(it_, 1);
  }

private:
  // Position of the read cursor within `input_`, for the caret line in parse errors.
  std::size_t offset() const
  {
    return static_cast<std::size_t>(std::distance(std::begin(input_), it_));
  }

  std::string_view input_;
  std::string_view::iterator it_;
};

void ParseCommonDateAndTime(StringParser& parser, std::tm& tm)
{
  // YYYY-MM-DDThh:mm:ssZ
  // ^^^^
  // Adjust year to be relative to 1900.
  tm.tm_year = parser.ExtractInt(4).InRange(0, 9999).value() - 1900;

  // YYYY-MM-DDThh:mm:ssZ
  //     ^
  parser.Expect('-');

  // YYYY-MM-DDThh:mm:ssZ
  //      ^^
  // Adjust month to be zero-based.
  tm.tm_mon = parser.ExtractInt(2).InRange(1, 12).value() - 1;

  // YYYY-MM-DDThh:mm:ssZ
  //        ^
  parser.Expect('-');

  // YYYY-MM-DDThh:mm:ssZ
  //         ^^
  tm.tm_mday = parser.ExtractInt(2).InRange(1, 31).value();

  // YYYY-MM-DDThh:mm:ssZ
  //           ^
  parser.Expect('T');

  // YYYY-MM-DDThh:mm:ssZ
  //            ^^
  tm.tm_hour = parser.ExtractInt(2).InRange(0, 24).value();

  // YYYY-MM-DDThh:mm:ssZ
  //              ^
  parser.Expect(':');

  // YYYY-MM-DDThh:mm:ssZ
  //               ^^
  tm.tm_min = parser.ExtractInt(2).InRange(0, 59).value();

  // YYYY-MM-DDThh:mm:ssZ
  //                 ^
  parser.Expect(':');

  // YYYY-MM-DDThh:mm:ssZ
  //                  ^^
  // 60 is used to denote an added leap second.
  tm.tm_sec = parser.ExtractInt(2).InRange(0, 60).value();
}

std::chrono::minutes ParseTimezone(StringParser& parser)
{
  std::chrono::minutes adjustment(0);

  // YYYY-MM-DDThh:mm:ss±hh:mm
  //                    ^
  const auto ch = parser.ExtractChar().OneOf('+', '-').value();
  const int sign = (ch == '+') ? -1 : 1;

  // YYYY-MM-DDThh:mm:ss±hh:mm
  //                     ^^
  const auto hours = parser.ExtractInt(2).InRange(0, 24).value();
  adjustment += std::chrono::hours(hours * sign);

  // YYYY-MM-DDThh:mm:ss±hh:mm
  //                        ^^
  parser.Expect(':');

  // YYYY-MM-DDThh:mm:ss±hh:mm
  //                        ^^
  const auto mins = parser.ExtractInt(2).InRange(0, 59).value();
  adjustment += std::chrono::minutes(mins * sign);

  return adjustment;
}

} // namespace

std::chrono::system_clock::time_point parse_gpx_time(std::string_view time_str)
{
  // https://en.cppreference.com/w/cpp/chrono/c/tm
  // https://www.gnu.org/software/libc/manual/html_node/Broken_002ddown-Time.html
  // https://man7.org/linux/man-pages/man3/tm.3type.html
  // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/localtime-s-localtime32-s-localtime64-s?view=msvc-170
  std::tm tm{};
  tm.tm_mday = 1; // Unlike the other members, this starts at 1.

  std::chrono::milliseconds adjustment(0);

  StringParser parser(time_str);

  // Ordered by assumed likelihood.
  switch (time_str.size())
  {
  case Iso8601FormatLength::DateTimeZulu:
  {
    // YYYY-MM-DDThh:mm:ssZ
    // ^^^^^^^^^^^^^^^^^^^
    ParseCommonDateAndTime(parser, tm);

    // YYYY-MM-DDThh:mm:ssZ
    //                    ^
    parser.Expect('Z');
    break;
  }
  case Iso8601FormatLength::DateTimeMilliSecondsZulu:
  {
    // YYYY-MM-DDThh:mm:ss.sssZ
    // ^^^^^^^^^^^^^^^^^^^
    ParseCommonDateAndTime(parser, tm);

    // YYYY-MM-DDThh:mm:ss.sssZ
    //                    ^
    parser.ExtractChar().OneOf(',', '.');

    // YYYY-MM-DDThh:mm:ss.sssZ
    //                     ^^^
    const auto ms = parser.ExtractInt(3).value();
    adjustment += std::chrono::milliseconds(ms);

    // YYYY-MM-DDThh:mm:ss.sssZ
    //                        ^
    parser.Expect('Z');
    break;
  }
  case Iso8601FormatLength::DateTimeTimeZone:
  {
    // YYYY-MM-DDThh:mm:ss±hh:mm
    // ^^^^^^^^^^^^^^^^^^^
    ParseCommonDateAndTime(parser, tm);

    // YYYY-MM-DDThh:mm:ss±hh:mm
    //                    ^^^^^^
    adjustment += ParseTimezone(parser);
    break;
  }
  case Iso8601FormatLength::DateTimeMilliSecondsTimeZone:
  {
    // YYYY-MM-DDThh:mm:ss.sss±hh:mm
    // ^^^^^^^^^^^^^^^^^^^
    ParseCommonDateAndTime(parser, tm);

    // YYYY-MM-DDThh:mm:ss.sss±hh:mm
    //                    ^
    parser.ExtractChar().OneOf(',', '.');

    // YYYY-MM-DDThh:mm:ss.sss±hh:mm
    //                     ^^^
    const auto ms = parser.ExtractInt(3).value();
    adjustment += std::chrono::milliseconds(ms);

    // YYYY-MM-DDThh:mm:ss.sss±hh:mm
    //                        ^^^^^^
    adjustment += ParseTimezone(parser);
    break;
  }
  case Iso8601FormatLength::DateTimeNoTimezone:
  {
    // YYYY-MM-DDThh:mm:ss
    // ^^^^^^^^^^^^^^^^^^^
    ParseCommonDateAndTime(parser, tm);
    break;
  }
  case Iso8601FormatLength::DateTimeMilliSecondsNoTimezone:
  {
    // YYYY-MM-DDThh:mm:ss.sss
    // ^^^^^^^^^^^^^^^^^^^
    ParseCommonDateAndTime(parser, tm);

    // YYYY-MM-DDThh:mm:ss.sss
    //                    ^
    parser.ExtractChar().OneOf(',', '.');

    // YYYY-MM-DDThh:mm:ss.sss
    //                     ^^^
    const auto ms = parser.ExtractInt(3).value();
    adjustment += std::chrono::milliseconds(ms);
    break;
  }
  default:
    throw parse_error("invalid or unexpected format");
  }

  return to_system_clock_time(&tm, adjustment, time_str);
}

} // namespace fastgpx
