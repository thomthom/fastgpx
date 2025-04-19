#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "fastgpx/errors.hpp"

TEST_CASE("parse_error error message", "[errors]")
{
  const std::string_view source("Example source string");
  const auto range = source.substr(8, 6);

  const std::string message("Hello World");
  const fastgpx::parse_error error(message, source, range);

  const std::string expected(
      "Hello World\n"
      "  \"Example source string\"\n"
      "           ^^^^^^");
  const auto actual = error.what();
  CHECK(actual == expected);
}
