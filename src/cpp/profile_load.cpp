// Profiling workload: calls fastgpx::LoadGpx on every .gpx file under a folder, several passes,
// and does nothing else, so a profiler attached to it sees only LoadGpx and what it calls.
// benchmarks/load_profile.md describes how it was used.
//
//   profile_load <folder> [passes]
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <print>
#include <string_view>
#include <system_error>
#include <vector>

#include "fastgpx/fastgpx.hpp"

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::println(stderr, "usage: profile_load <folder> [passes]");
    return 1;
  }

  int passes = 10;
  if (argc > 2)
  {
    const std::string_view text = argv[2];
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), passes);
    if (ec != std::errc{} || end != text.data() + text.size() || passes < 1)
    {
      std::println(stderr, "profile_load: passes must be a positive integer, got '{}'", text);
      return 1;
    }
  }

  const std::filesystem::path folder = argv[1];
  std::error_code error;
  if (!std::filesystem::is_directory(folder, error))
  {
    std::println(stderr, "profile_load: '{}' is not a folder", folder.string());
    return 1;
  }

  std::vector<std::filesystem::path> files;
  for (auto it = std::filesystem::recursive_directory_iterator(folder, error);
       !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error))
  {
    if (it->path().extension() == ".gpx")
    {
      files.push_back(it->path());
    }
  }
  if (error)
  {
    std::println(stderr, "profile_load: cannot list '{}': {}", folder.string(), error.message());
    return 1;
  }
  std::sort(files.begin(), files.end());

  // Counting the points uses each result, so the loads cannot be optimised away.
  // The try block wraps the whole loop, so the only extra work inside the loop is remembering the
  // current file for the error message.
  std::size_t points = 0;
  const std::filesystem::path* current = nullptr;
  const auto start = std::chrono::steady_clock::now();
  try
  {
    for (int pass = 0; pass < passes; ++pass)
    {
      for (const auto& file : files)
      {
        current = &file;
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
  }
  catch (const std::exception& e)
  {
    std::println(stderr, "profile_load: {}: {}", current ? current->string() : folder.string(),
                 e.what());
    return 1;
  }
  const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

  if (points == 0)
  {
    std::println(stderr, "profile_load: no track points in {} .gpx files under '{}'", files.size(),
                 folder.string());
    return 1;
  }

  std::println("{} files x {} passes, {} points, {:.2f} s, {:.0f} ns/point", files.size(), passes,
               points, elapsed.count(), elapsed.count() * 1e9 / static_cast<double>(points));
}
