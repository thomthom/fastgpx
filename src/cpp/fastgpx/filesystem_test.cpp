#include <array>
#include <filesystem>
#include <stdexcept>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "fastgpx/filesystem.hpp"

const auto project_path = std::filesystem::path(FASTGPX_PROJECT_DIR);

TEST_CASE("Open file with ascii path for binary reading", "[filesystem]")
{
  const auto path = project_path / "gpx/2024 TopCamp/Connected_20240518_094959_.gpx";

  const auto file = fastgpx::open_file(path);
  CHECK(file != nullptr);

  std::array<char, 5> buffer;
  const auto result = fread(buffer.data(), sizeof(char), buffer.size(), file);
  CHECK(result == buffer.size());

  CHECK(fclose(file) == 0);
}

#ifdef _WIN32

// TODO: Test unicode path.
//   UTF-8 encoding in C++ source files can be a pain.

TEST_CASE("Convert UTF-8 string to UTF-16 string", "[filesystem][unicode]")
{
  using namespace std::string_literals;

  // C++20 support for std::u8string is lacking...
  // https://stackoverflow.com/a/59055485
  //                    "æ" == "\xC3\xA6"
  const std::string utf8("Hello \xC3\xA6 World");
  //                                  "æ" == "\u00E6"
  const std::wstring expected_utf16 = L"Hello \u00E6 World";

  const auto actual_utf16 = fastgpx::utf8_to_utf16(utf8);
  CHECK(actual_utf16 == expected_utf16);
}

TEST_CASE("Convert empty UTF-8 string to UTF-16 string", "[filesystem][unicode]")
{
  const std::string utf8 = "";
  const std::wstring expected_utf16 = L"";

  const auto actual_utf16 = fastgpx::utf8_to_utf16(utf8);
  CHECK(actual_utf16 == expected_utf16);
}

TEST_CASE("Convert invalid UTF-8 string to UTF-16 string", "[filesystem][unicode]")
{
  const std::string utf8 = "Hello \xF5\xF6\xF7\xFF World";
  REQUIRE_THROWS_AS(fastgpx::utf8_to_utf16(utf8), std::runtime_error);
}

#endif // _WIN32
