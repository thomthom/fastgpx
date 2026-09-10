#include "fastgpx/datetime.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <string_view>
#include <system_error>
#include <type_traits>

#include "fastgpx/errors.hpp"

namespace fastgpx {
namespace {

// The calendar fields of a timestamp as they are written in the string. `hour` may be 24 and
// `second` may be 60, and `day` may be past the end of the month; the conversion below carries
// each of those into the following day, minute or month.
struct CivilTime
{
  int year = 0;
  int month = 1;
  int day = 1;
  int hour = 0;
  int minute = 0;
  int second = 0;
};

// Converts a civil UTC time, a timezone offset and a sub-second fraction to a `system_clock`
// time point, rejecting anything that cannot be represented.
//
// The date arithmetic is `std::chrono`'s rather than `timegm`/`_mkgmtime`, which cost a call
// into the C runtime and, on Windows, rejected everything outside 1970..3000. See #19.
// `sys_days` carries a day past the end of the month into the next month the way `timegm` did,
// so 2008-02-31 still reads as 2008-03-02.
//
// Two range checks stand between the string and the result:
//
// - `system_clock::duration` is nanoseconds on libstdc++, so dates outside roughly
//   1677-09-21..2262-04-11 overflow `int64_t`, which is undefined behaviour. Found by fuzzing
//   (#48). Other implementations use coarser ticks and reach much further.
// - The result has to stay within a four digit year, which is all the format can write and all
//   that `datetime.datetime` in the Python bindings can hold. The year in the string is already
//   [0, 9999], but year 0 is not a year and a timezone offset can push either end past the
//   boundary.
std::chrono::system_clock::time_point to_system_clock_time(const CivilTime& civil,
                                                           std::chrono::minutes offset,
                                                           std::chrono::nanoseconds fraction,
                                                           std::string_view time_str)
{
  using namespace std::chrono;
  using sys_duration = system_clock::duration;

  // The parser has already restricted the fields to the ranges the conversion to `sys_days` is
  // defined for: years [0, 9999], months [1, 12] and days [1, 31].
  const year_month_day date(year(civil.year), month(static_cast<unsigned>(civil.month)),
                            day(static_cast<unsigned>(civil.day)));
  const auto days_since_epoch = static_cast<sys_days>(date).time_since_epoch();
  const auto since_epoch = duration_cast<seconds>(days_since_epoch) + hours(civil.hour) +
                           minutes(civil.minute) + seconds(civil.second) + offset;

  // The fraction is never negative and always less than a second, so it can only carry
  // `since_epoch` one second forward.
  constexpr auto margin = seconds(1);
  constexpr auto max_seconds = floor<seconds>(sys_duration::max()) - margin;
  constexpr auto min_seconds = ceil<seconds>(sys_duration::min());
  if (since_epoch > max_seconds || since_epoch < min_seconds)
  {
    throw parse_error(std::format("time is outside the range of system_clock: \"{}\"", time_str));
  }

  constexpr auto first_year =
      duration_cast<seconds>(sys_days(year(1) / January / 1).time_since_epoch());
  constexpr auto after_last_year =
      duration_cast<seconds>(sys_days(year(10000) / January / 1).time_since_epoch());
  if (since_epoch < first_year || since_epoch >= after_last_year)
  {
    throw parse_error(std::format("time is outside a four digit year: \"{}\"", time_str));
  }

  // The fraction is truncated to the clock's resolution on its own. Truncating it together with
  // a negative timezone offset would round the sum towards zero, so digits below the resolution
  // would round up instead of being dropped.
  return system_clock::time_point(duration_cast<sys_duration>(since_epoch)) +
         duration_cast<sys_duration>(fraction);
}

// Reads `count` decimal digits starting at `offset`, or -1 if any of them is not a digit.
// `offset + count` must be within `time_str`.
int ReadDigits(std::string_view time_str, std::size_t offset, std::size_t count)
{
  int value = 0;
  for (std::size_t i = 0; i < count; ++i)
  {
    const auto ch = static_cast<unsigned char>(time_str[offset + i]);
    if (ch < '0' || ch > '9')
    {
      return -1;
    }
    value = (value * 10) + (ch - '0');
  }
  return value;
}

// The two forms that dominate GPX files get a fixed-position fast path; see `parse_gpx_time`.
enum Iso8601FormatLength
{
  DateTimeZulu = 20,             // YYYY-MM-DDThh:mm:ssZ
  DateTimeMilliSecondsZulu = 24, // YYYY-MM-DDThh:mm:ss.sssZ
};

// Fractional seconds are kept to nanosecond precision; further digits are consumed and dropped.
constexpr std::size_t kMaxFractionDigits = 9;

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

  // Consumes one or more digits after the fraction separator and returns them as a duration.
  // xsd:dateTime puts no limit on the number of digits, so any beyond `kMaxFractionDigits` are
  // consumed but do not contribute.
  std::chrono::nanoseconds ExtractFraction()
  {
    std::int64_t value = 0;
    std::size_t digits = 0;
    while (it_ != std::end(input_) && std::isdigit(static_cast<unsigned char>(*it_)) != 0)
    {
      if (digits < kMaxFractionDigits)
      {
        value = value * 10 + (*it_ - '0');
      }
      ++digits;
      std::advance(it_, 1);
    }
    if (digits == 0)
    {
      throw parse_error("expected fractional second digits", input_, offset(), 1);
    }
    for (auto scale = std::min(digits, kMaxFractionDigits); scale < kMaxFractionDigits; ++scale)
    {
      value *= 10;
    }
    return std::chrono::nanoseconds(value);
  }

  bool AtEnd() const { return it_ == std::end(input_); }

  // The character at the read cursor, or NUL at the end of the string.
  char Peek() const { return AtEnd() ? '\0' : *it_; }

  void ExpectEnd()
  {
    if (!AtEnd())
    {
      const auto remaining = static_cast<std::size_t>(std::distance(it_, std::end(input_)));
      throw parse_error("unexpected characters after the time", input_, offset(), remaining);
    }
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

void ParseCommonDateAndTime(StringParser& parser, CivilTime& civil)
{
  // YYYY-MM-DDThh:mm:ssZ
  // ^^^^
  civil.year = parser.ExtractInt(4).InRange(0, 9999).value();

  // YYYY-MM-DDThh:mm:ssZ
  //     ^
  parser.Expect('-');

  // YYYY-MM-DDThh:mm:ssZ
  //      ^^
  civil.month = parser.ExtractInt(2).InRange(1, 12).value();

  // YYYY-MM-DDThh:mm:ssZ
  //        ^
  parser.Expect('-');

  // YYYY-MM-DDThh:mm:ssZ
  //         ^^
  civil.day = parser.ExtractInt(2).InRange(1, 31).value();

  // YYYY-MM-DDThh:mm:ssZ
  //           ^
  parser.Expect('T');

  // YYYY-MM-DDThh:mm:ssZ
  //            ^^
  civil.hour = parser.ExtractInt(2).InRange(0, 24).value();

  // YYYY-MM-DDThh:mm:ssZ
  //              ^
  parser.Expect(':');

  // YYYY-MM-DDThh:mm:ssZ
  //               ^^
  civil.minute = parser.ExtractInt(2).InRange(0, 59).value();

  // YYYY-MM-DDThh:mm:ssZ
  //                 ^
  parser.Expect(':');

  // YYYY-MM-DDThh:mm:ssZ
  //                  ^^
  // 60 is used to denote an added leap second.
  civil.second = parser.ExtractInt(2).InRange(0, 60).value();
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
  CivilTime civil;
  std::chrono::minutes offset(0);
  std::chrono::nanoseconds fraction(0);

  StringParser parser(time_str);

  // A string of one of the two Zulu lengths that ends in 'Z' can only be that form, so it takes
  // the fixed-position path. Everything else goes through the general path, which accepts any
  // number of fractional digits and an optional 'Z' or ±hh:mm offset.
  const bool zulu = !time_str.empty() && time_str.back() == 'Z';
  if (zulu && time_str.size() == Iso8601FormatLength::DateTimeZulu)
  {
    // YYYY-MM-DDThh:mm:ssZ
    // ^^^^^^^^^^^^^^^^^^^
    ParseCommonDateAndTime(parser, civil);

    // YYYY-MM-DDThh:mm:ssZ
    //                    ^
    parser.Expect('Z');
  }
  else if (zulu && time_str.size() == Iso8601FormatLength::DateTimeMilliSecondsZulu)
  {
    // YYYY-MM-DDThh:mm:ss.sssZ
    // ^^^^^^^^^^^^^^^^^^^
    ParseCommonDateAndTime(parser, civil);

    // YYYY-MM-DDThh:mm:ss.sssZ
    //                    ^
    parser.ExtractChar().OneOf(',', '.');

    // YYYY-MM-DDThh:mm:ss.sssZ
    //                     ^^^
    const auto ms = parser.ExtractInt(3).value();
    fraction = std::chrono::milliseconds(ms);

    // YYYY-MM-DDThh:mm:ss.sssZ
    //                        ^
    parser.Expect('Z');
  }
  else
  {
    // YYYY-MM-DDThh:mm:ss[.s...][Z|±hh:mm]
    // ^^^^^^^^^^^^^^^^^^^
    ParseCommonDateAndTime(parser, civil);

    // YYYY-MM-DDThh:mm:ss[.s...][Z|±hh:mm]
    //                     ^^^^^
    if (parser.Peek() == '.' || parser.Peek() == ',')
    {
      parser.ExtractChar();
      fraction = parser.ExtractFraction();
    }

    // YYYY-MM-DDThh:mm:ss[.s...][Z|±hh:mm]
    //                            ^^^^^^^^
    // No designator at all is read as UTC, which is what GPX specifies for <time>.
    if (parser.Peek() == 'Z')
    {
      parser.ExtractChar();
    }
    else if (!parser.AtEnd())
    {
      offset = ParseTimezone(parser);
    }
    parser.ExpectEnd();
  }

  return to_system_clock_time(civil, offset, fraction, time_str);
}

bool is_sortable_gpx_time(std::string_view time_str)
{
  const auto size = time_str.size();
  const bool has_fraction = (size == Iso8601FormatLength::DateTimeMilliSecondsZulu);
  if (size != Iso8601FormatLength::DateTimeZulu && !has_fraction)
  {
    return false;
  }

  // YYYY-MM-DDThh:mm:ss[.sss]Z
  //     ^  ^  ^  ^  ^      ^ ^
  if (time_str[4] != '-' || time_str[7] != '-' || time_str[10] != 'T' || time_str[13] != ':' ||
      time_str[16] != ':' || time_str.back() != 'Z')
  {
    return false;
  }
  if (has_fraction)
  {
    // `parse_gpx_time` also accepts ',' as the separator. Two strings that differ only there
    // would be ordered by the separator rather than by the fraction, so only '.' is sortable.
    if (time_str[19] != '.' || ReadDigits(time_str, 20, 3) < 0)
    {
      return false;
    }
  }

  const auto year = ReadDigits(time_str, 0, 4);
  const auto month = ReadDigits(time_str, 5, 2);
  const auto day = ReadDigits(time_str, 8, 2);
  const auto hour = ReadDigits(time_str, 11, 2);
  const auto minute = ReadDigits(time_str, 14, 2);
  const auto second = ReadDigits(time_str, 17, 2);
  if (year < 0 || month < 0 || day < 0 || hour < 0 || minute < 0 || second < 0)
  {
    return false;
  }

  // `parse_gpx_time` accepts hour 24 and second 60, which name a time in the following day or
  // minute. Both sort before strings that are chronologically earlier, so they are not sortable.
  if (hour > 23 || minute > 59 || second > 59)
  {
    return false;
  }

  // A day past the end of its month carries into the next month, with the same problem.
  const std::chrono::year_month_day date(std::chrono::year(year),
                                         std::chrono::month(static_cast<unsigned>(month)),
                                         std::chrono::day(static_cast<unsigned>(day)));
  return date.ok();
}

} // namespace fastgpx
