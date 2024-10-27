#pragma once

#include <filesystem>
#include <vector>

namespace fastgpx {

struct ExpectedSegment
{
  double length2d; // Meters
  double length3d; // Meters
};

struct ExpectedTrack
{
  std::vector<ExpectedSegment> segments;
  double length2d; // Meters
  double length3d; // Meters
};

struct ExpectedGpx
{
  std::filesystem::path path;
  std::vector<ExpectedTrack> tracks;
  double length2d; // Meters
  double length3d; // Meters
};

std::vector<ExpectedGpx> LoadExpectedGpxData(const std::filesystem::path json_path);

} // namespace fastgpx
