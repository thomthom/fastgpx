#include "fastgpx/test_data.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include <nlohmann/json.hpp>

#include "fastgpx/fastgpx.hpp"

using json = nlohmann::json;

namespace fastgpx {

void from_json(const json& json, TimeBounds& bounds)
{
  int start_time, end_time;
  json.at("start_time").get_to(start_time);
  json.at("end_time").get_to(end_time);
  bounds.start_time = std::chrono::system_clock::from_time_t(start_time);
  bounds.end_time = std::chrono::system_clock::from_time_t(end_time);
}

void from_json(const json& json, ExpectedSegment& segment)
{
  json.at("length2d").get_to(segment.length2d);
  json.at("length3d").get_to(segment.length3d);
  if (json.contains("time_bounds"))
    json.at("time_bounds").get_to(segment.time_bounds);
}

void from_json(const json& json, ExpectedTrack& track)
{
  json.at("length2d").get_to(track.length2d);
  json.at("length3d").get_to(track.length3d);
  json.at("segments").get_to(track.segments);
  if (json.contains("time_bounds"))
    json.at("time_bounds").get_to(track.time_bounds);
}

void from_json(const json& json, ExpectedGpx& gpx)
{
  json.at("path").get_to(gpx.path);
  json.at("length2d").get_to(gpx.length2d);
  json.at("length3d").get_to(gpx.length3d);
  json.at("tracks").get_to(gpx.tracks);
  if (json.contains("time_bounds"))
    json.at("time_bounds").get_to(gpx.time_bounds);
}

std::vector<ExpectedGpx> LoadExpectedGpxData(const std::filesystem::path json_path)
{
  std::ifstream json_file(json_path);
  if (!json_file.is_open())
  {
    throw "Could not open the file: " + json_path.string() + '\n';
  }

  json parsed_json;
  json_file >> parsed_json;

  const auto gpx_data = parsed_json.get<std::vector<ExpectedGpx>>();
  return gpx_data;
}

} // namespace fastgpx
