#pragma once

#include <filesystem>
#include <tuple>
#include <string>
#include <vector>

namespace fastgpx
{

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
    std::vector<ExpectedTrack> tracks;
    double length2d; // Meters
    double length3d; // Meters
  };

  using ExpectedGpxData = std::tuple<std::string, ExpectedGpx>;

  std::vector<ExpectedGpxData> LoadExpectedGpxData(const std::filesystem::path json_path);

} // namespace
