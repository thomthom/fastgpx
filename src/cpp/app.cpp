#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <numbers>
#include <print>

#include "fastgpx/fastgpx.hpp"

int main()
{
  std::println("fastgpx");

  const auto relative_path = std::filesystem::path("../../../../gpx/test");
  auto path = std::filesystem::absolute(relative_path);
  path.append("debug-segment.gpx");
  std::println("{}", path.string());

  if (!std::filesystem::exists(path))
  {
    std::println("File not found.");
    return 0;
  }

  std::println("");
  const auto gpx = fastgpx::ParseGpx(path);
  const auto distance = gpx.GetLength2D();
  std::println("Distance 2D: {}", distance);

  return 0;
}
