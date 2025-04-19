#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "fastgpx/fastgpx.hpp"

namespace fastgpx {

struct ExpectedSegment
{
  double length2d; // Meters
  double length3d; // Meters
  TimeBounds time_bounds;
};

struct ExpectedTrack
{
  std::vector<ExpectedSegment> segments;
  double length2d; // Meters
  double length3d; // Meters
  TimeBounds time_bounds;
};

struct ExpectedGpx
{
  std::filesystem::path path;
  std::vector<ExpectedTrack> tracks;
  double length2d; // Meters
  double length3d; // Meters
  TimeBounds time_bounds;
};

std::vector<ExpectedGpx> LoadExpectedGpxData(const std::filesystem::path json_path);

} // namespace fastgpx
