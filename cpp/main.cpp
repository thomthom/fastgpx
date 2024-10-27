#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <numbers>
#include <print>

#include "gpx.hpp"

int main()
{
  std::println("GPX Reader");

  const auto relative_path = std::filesystem::path("../../gpx/2024 Great Roadtrip");
  const auto path = std::filesystem::absolute(relative_path);
  std::println("{}", path.string());

  if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
  {
    std::println("Directory not found or is not a directory.");
    return 0;
  }

  // tinyxml2
  std::println("");
  std::println("tinyxml2");
  {
    auto start = std::chrono::high_resolution_clock::now();
    double total_length = 0.0;
    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
      if (entry.path().extension() != ".gpx")
      {
        continue;
      }
      total_length += tinyxml_gpx_length2d(entry.path());
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::println("Total Length: {}", total_length);
    std::println("Elapsed time: {} seconds", duration.count());
  }

  // pugixml
  std::println("");
  std::println("pugixml");
  {
    auto start = std::chrono::high_resolution_clock::now();
    double total_length = 0.0;
    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
      if (entry.path().extension() != ".gpx")
      {
        continue;
      }
      total_length += pugixml_gpx_length2d(entry.path());
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::println("Total Length: {}", total_length);
    std::println("Elapsed time: {} seconds", duration.count());
  }

  return 0;
}
