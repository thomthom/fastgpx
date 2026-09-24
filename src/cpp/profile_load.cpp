// Profiling workload: calls fastgpx::LoadGpx on every .gpx file under a folder, several passes,
// and does nothing else, so a profiler attached to it sees only LoadGpx and what it calls.
// benchmarks/load_profile.md describes how it was used.
//
//   profile_load <folder> [passes]
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <print>
#include <string>
#include <vector>

#include "fastgpx/fastgpx.hpp"

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::println("usage: profile_load <folder> [passes]");
    return 1;
  }
  const int passes = argc > 2 ? std::stoi(argv[2]) : 10;

  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(argv[1]))
  {
    if (entry.path().extension() == ".gpx")
    {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());

  // Counting the points uses each result, so the loads cannot be optimised away.
  std::size_t points = 0;
  const auto start = std::chrono::steady_clock::now();
  for (int pass = 0; pass < passes; ++pass)
  {
    for (const auto& file : files)
    {
      const auto gpx = fastgpx::LoadGpx(file);
      for (const auto& track : gpx.tracks)
      {
        for (const auto& segment : track.segments)
        {
          points += segment.points.size();
        }
      }
    }
  }
  const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

  std::println("{} files x {} passes, {} points, {:.2f} s, {:.0f} ns/point", files.size(), passes,
               points, elapsed.count(), elapsed.count() * 1e9 / static_cast<double>(points));
}
