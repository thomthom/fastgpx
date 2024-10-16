#include "test_data.hpp"

#include "fastgpx.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fastgpx
{

  struct ExpectedGpxInfo
  {
    std::string path;
    ExpectedGpx data;
  };

  void from_json(const json &json, ExpectedSegment &segment)
  {
    json.at("length2d").get_to(segment.length2d);
    json.at("length3d").get_to(segment.length3d);
  }

  void from_json(const json &json, ExpectedTrack &track)
  {
    json.at("length2d").get_to(track.length2d);
    json.at("length3d").get_to(track.length3d);
    json.at("segments").get_to(track.segments);
  }

  void from_json(const json &json, ExpectedGpx &gpx)
  {
    json.at("length2d").get_to(gpx.length2d);
    json.at("length3d").get_to(gpx.length3d);
    json.at("tracks").get_to(gpx.tracks);
  }

  void from_json(const json &json, ExpectedGpxInfo &info)
  {
    json.at("path").get_to(info.path);
    json.at("data").get_to(info.data);
  }

  std::vector<ExpectedGpxData> LoadExpectedGpxData(const std::filesystem::path json_path)
  {
    std::ifstream json_file(json_path);
    if (!json_file.is_open())
    {
      throw "Could not open the file: " + json_path.string() + '\n';
    }

    json parsed_json;
    json_file >> parsed_json;

    const std::vector<ExpectedGpxInfo> gpx_data = parsed_json.get<std::vector<ExpectedGpxInfo>>();

    // TODO: Consider avoiding this by moving "path" to ExpectedGpx struct.
    std::vector<ExpectedGpxData> test_data(gpx_data.size());
    std::ranges::transform(gpx_data, test_data.begin(), [](const auto &info)
                           { return ExpectedGpxData{info.path, info.data}; });
    return test_data;
  }

} // namespace fastgpx
