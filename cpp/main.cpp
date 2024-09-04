#include <cmath>
#include <chrono>
#include <filesystem>
#include <format>
#include <numbers>
#include <print>

#include <tinyxml2.h>

#include <pugixml.hpp>


double haversine(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371e3; // Earth radius in meters
    const double to_radians = std::numbers::pi / 180.0;

    lat1 *= to_radians;
    lon1 *= to_radians;
    lat2 *= to_radians;
    lon2 *= to_radians;

    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double a = sin(dlat / 2) * sin(dlat / 2) +
               cos(lat1) * cos(lat2) * sin(dlon / 2) * sin(dlon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return R * c; // Distance in meters
}


double tinyxml_gpx_length(const std::filesystem::path path) {

  auto doc = tinyxml2::XMLDocument();
  doc.LoadFile(path.string().c_str());

  auto root = doc.FirstChildElement("gpx");

  double total_distance = 0.0;

  for (auto track = root->FirstChildElement("trk"); track; track = track->NextSiblingElement("trk")) {
    for (auto segment = track->FirstChildElement("trkseg"); segment; segment = segment->NextSiblingElement("trkseg")) {
      double segment_distance = 0.0;

      tinyxml2::XMLElement* prev_trkpt = nullptr;
      // Iterate over each <trkpt> in the segment
      for (auto trkpt = segment->FirstChildElement("trkpt"); trkpt; trkpt = trkpt->NextSiblingElement("trkpt")) {
        if (prev_trkpt) {
          // Get current trackpoint latitude and longitude
          double lat1 = std::stod(prev_trkpt->Attribute("lat"));
          double lon1 = std::stod(prev_trkpt->Attribute("lon"));
          double lat2 = std::stod(trkpt->Attribute("lat"));
          double lon2 = std::stod(trkpt->Attribute("lon"));

          // Compute the distance between two track points
          segment_distance += haversine(lat1, lon1, lat2, lon2);
        }
        prev_trkpt = trkpt;
      }
      total_distance += segment_distance;
    }
  }

  return total_distance ;
}


double pugixml_gpx_length(const std::filesystem::path& path) {
  pugi::xml_document doc;

  // Load the GPX file
  pugi::xml_parse_result result = doc.load_file(path.string().c_str());
  if (!result) {
      // std::cerr << "Failed to load GPX file: " << result.description() << "\n";
      return 0.0;
  }

  pugi::xml_node root = doc.child("gpx");

  double total_distance = 0.0;

  // Iterate over each <trk> element
  for (pugi::xml_node track = root.child("trk"); track; track = track.next_sibling("trk")) {
      // Iterate over each <trkseg> element
      for (pugi::xml_node segment = track.child("trkseg"); segment; segment = segment.next_sibling("trkseg")) {
          double segment_distance = 0.0;

          pugi::xml_node prev_trkpt;
          // Iterate over each <trkpt> in the segment
          for (pugi::xml_node trkpt = segment.child("trkpt"); trkpt; trkpt = trkpt.next_sibling("trkpt")) {
              if (prev_trkpt) {
                  // Get current and previous trackpoint lat/lon attributes
                  double lat1 = prev_trkpt.attribute("lat").as_double();
                  double lon1 = prev_trkpt.attribute("lon").as_double();
                  double lat2 = trkpt.attribute("lat").as_double();
                  double lon2 = trkpt.attribute("lon").as_double();

                  // Compute distance between two points
                  segment_distance += haversine(lat1, lon1, lat2, lon2);
              }
              prev_trkpt = trkpt;
          }
          total_distance += segment_distance;
      }
  }

  return total_distance;
}

int main() {
  std::println("GPX Reader");

  const auto relative_path = std::filesystem::path("../../gpx/2024 Great Roadtrip");
  const auto path = std::filesystem::absolute(relative_path);
  std::println("{}", path.string());

  if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
    std::println("Directory not found or is not a directory.");
    return 0;
  }

  // tinyxml2
  std::println("tinyxml2");
  {
    auto start = std::chrono::high_resolution_clock::now();
    double total_length = 0.0;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      if (entry.path().extension() != ".gpx") {
        continue;
      }
      total_length += tinyxml_gpx_length(entry.path());
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::println("Total Length: {}", total_length);
    std::println("Elapsed time: {} seconds", duration.count());
  }

  // pugixml
  std::println("pugixml");
  {
    auto start = std::chrono::high_resolution_clock::now();
    double total_length = 0.0;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      if (entry.path().extension() != ".gpx") {
        continue;
      }
      total_length += pugixml_gpx_length(entry.path());
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::println("Total Length: {}", total_length);
    std::println("Elapsed time: {} seconds", duration.count());
  }

  return 0;
}
