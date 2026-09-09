#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "fastgpx/errors.hpp"

TEST_CASE("parse_error error message", "[errors]")
{
  const std::string_view source("Example source string");

  const std::string message("Hello World");
  const fastgpx::parse_error error(message, source, 8, 6);

  const std::string expected(
      "Hello World\n"
      "  \"Example source string\"\n"
      "           ^^^^^^");
  const auto actual = error.what();
  CHECK(actual == expected);
}

TEST_CASE("parse_error marker is clamped to the source string", "[errors]")
{
  // The marker takes an index, so a caller can name a position outside the source where it could
  // not pass a sub-view of it. Those values must not throw from the constructor, nor size the
  // padding by an arbitrary number. The sub-view form this replaced had the same failure for a
  // view pointing outside the source: the offset came out negative and `std::format` threw
  // `std::format_error` out of the `parse_error` constructor.
  const std::string_view source("0123456789");
  const std::string message("Out of range");

  SECTION("offset past the end")
  {
    const fastgpx::parse_error error(message, source, 25, 3);
    const std::string expected(
        "Out of range\n"
        "  \"0123456789\"\n"
        "             ");
    CHECK(error.what() == expected);
  }

  SECTION("length running past the end")
  {
    const fastgpx::parse_error error(message, source, 8, 99);
    const std::string expected(
        "Out of range\n"
        "  \"0123456789\"\n"
        "           ^^");
    CHECK(error.what() == expected);
  }

  SECTION("empty source")
  {
    const fastgpx::parse_error error(message, std::string_view(), 4, 2);
    const std::string expected(
        "Out of range\n"
        "  \"\"\n"
        "   ");
    CHECK(error.what() == expected);
  }
}
