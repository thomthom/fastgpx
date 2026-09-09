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
std::chrono::system_clock::time_point to_system_clock_time(
    std::tm* tm, std::chrono::system_clock::duration adjustment, std::string_view time_str)
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

  std::chrono::system_clock::duration adjustment(0);

  StringParser parser(time_str);

  // A string of one of the two Zulu lengths that ends in 'Z' can only be that form, so it takes
  // the fixed-position path. Everything else goes through the general path, which accepts any
  // number of fractional digits and an optional 'Z' or ±hh:mm offset.
  const bool zulu = !time_str.empty() && time_str.back() == 'Z';
  if (zulu && time_str.size() == Iso8601FormatLength::DateTimeZulu)
  {
    // YYYY-MM-DDThh:mm:ssZ
    // ^^^^^^^^^^^^^^^^^^^
    ParseCommonDateAndTime(parser, tm);

    // YYYY-MM-DDThh:mm:ssZ
    //                    ^
    parser.Expect('Z');
  }
  else if (zulu && time_str.size() == Iso8601FormatLength::DateTimeMilliSecondsZulu)
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
  }
  else
  {
    // YYYY-MM-DDThh:mm:ss[.s...][Z|±hh:mm]
    // ^^^^^^^^^^^^^^^^^^^
    ParseCommonDateAndTime(parser, tm);

    // YYYY-MM-DDThh:mm:ss[.s...][Z|±hh:mm]
    //                     ^^^^^
    if (parser.Peek() == '.' || parser.Peek() == ',')
    {
      parser.ExtractChar();
      // The fraction is truncated to the clock's resolution on its own. Truncating after the
      // timezone offset is added would round towards zero on a negative sum, so digits below
      // the resolution would round up instead of being dropped.
      adjustment +=
          std::chrono::duration_cast<std::chrono::system_clock::duration>(parser.ExtractFraction());
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
      adjustment += ParseTimezone(parser);
    }
    parser.ExpectEnd();
  }

  return to_system_clock_time(&tm, adjustment, time_str);
}

} // namespace fastgpx
