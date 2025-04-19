#include <filesystem>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "fastgpx/test_data.hpp"

using namespace fastgpx;

const auto project_path = std::filesystem::path(FASTGPX_PROJECT_DIR);

TEST_CASE("Load expected GPX data", "[testdata]")
{
  const auto json_path = project_path / "src/cpp/fastgpx/test_data.json";
  const auto expected_data = LoadExpectedGpxData(json_path);

  REQUIRE(expected_data.size() == 1);

  const auto expected_gpx = expected_data.at(0);

  CAPTURE(expected_gpx.path);

  REQUIRE(!expected_gpx.time_bounds.IsEmpty());

  // TimeBounds
  // https://www.timestamp-converter.com/
  // UNIX timestamp for 2024-05-29T07:19:16Z
  const auto expected_start = std::chrono::system_clock::from_time_t(1716967156);
  // UNIX timestamp for 2024-05-29T10:43:32Z
  const auto expected_end = std::chrono::system_clock::from_time_t(1716979412);

  CHECK(*expected_gpx.time_bounds.start_time == expected_start);
  CHECK(*expected_gpx.time_bounds.end_time == expected_end);
}
