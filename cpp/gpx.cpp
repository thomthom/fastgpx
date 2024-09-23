#include "gpx.hpp"

#include <cmath>
#include <chrono>
#include <filesystem>
#include <format>
#include <numbers>
#include <print>

#include <tinyxml2.h>

#include <pugixml.hpp>

#ifdef _WIN32
    #include <windows.h> // for _wfopen
    // #include <stdio.h>

    #include <string>
    #include <stdexcept>

    std::wstring utf8_to_utf16(const std::string& utf8_str) {
    if (utf8_str.empty()) {
        return std::wstring(); // Return an empty wide string if input is empty
    }

    // Step 1: Determine the size of the wide string buffer required
    int wide_char_count = MultiByteToWideChar(
        CP_UTF8,                // Source string is in UTF-8
        0,                      // No special flags
        utf8_str.c_str(),        // Input UTF-8 string
        static_cast<int>(utf8_str.size()), // Length of the input string
        nullptr,                // No output buffer yet
        0                       // Request size of the output buffer
    );

    if (wide_char_count == 0) {
        throw std::runtime_error("Error converting UTF-8 to UTF-16: MultiByteToWideChar failed.");
    }

    // Step 2: Allocate the necessary buffer
    std::wstring utf16_str(wide_char_count, 0);

    // Step 3: Perform the conversion
    int result = MultiByteToWideChar(
        CP_UTF8,                // Source string is in UTF-8
        0,                      // No special flags
        utf8_str.c_str(),        // Input UTF-8 string
        static_cast<int>(utf8_str.size()), // Length of the input string
        &utf16_str[0],          // Output buffer for the wide string
        wide_char_count          // Size of the output buffer
    );

    if (result == 0) {
        throw std::runtime_error("Error converting UTF-8 to UTF-16: MultiByteToWideChar failed.");
    }

    return utf16_str;
}
#endif

FILE* open_file(const std::filesystem::path& file_path) {
    FILE* file = nullptr;

#ifdef _WIN32
    // On Windows, open with wide string support
    const auto pathu16 = utf8_to_utf16(file_path.string());
    file = _wfopen(pathu16.c_str(), L"rb"); // Open for reading in binary mode
    // file = _wfopen(file_path.c_str(), L"rb"); // Open for reading in binary mode
#else
    // On other platforms, open with standard fopen
    file = fopen(file_path.string().c_str(), "rb"); // Open for reading in binary mode
#endif

    return file;
}

double haversine(double lat1, double lon1, double lat2, double lon2)
{
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

// double tinyxml_gpx_length(const std::filesystem::path& path) {
//   std::println("Open file...");
//   const auto cstr = path.string().c_str();
//   FILE* xmlFile = open_file(path);
//   std::println("file: {}", reinterpret_cast<size_t>(xmlFile));

//   std::println("Loading XML doc...");
//   std::println("path: {}", path.string().c_str());
//   auto doc = tinyxml2::XMLDocument();
//   doc.LoadFile(xmlFile);

//   std::println("Get root...");
//   auto root = doc.FirstChildElement("gpx");
//   std::println("root: {}", reinterpret_cast<size_t>(root));

//   fclose(xmlFile);

//   return 0.0;
// }

double tinyxml_gpx_length(const std::filesystem::path& path)
{
  FILE* xmlFile = open_file(path);

  // std::println("Loading XML doc...");
  // std::println("path: {}", path.string().c_str());
  auto doc = tinyxml2::XMLDocument();
  doc.LoadFile(xmlFile);
  // doc.LoadFile(path.string().c_str());
  // doc.Parse(gpxdata.c_str(), gpxdata.size());

  // std::println("Get root...");
  auto root = doc.FirstChildElement("gpx");
  // std::println("root: {}", reinterpret_cast<size_t>(root));

  double total_distance = 0.0;

  // std::println("Iterate trk...");
  for (auto track = root->FirstChildElement("trk"); track; track = track->NextSiblingElement("trk"))
  {
    // std::println("Iterate trkseg...");
    for (auto segment = track->FirstChildElement("trkseg"); segment; segment = segment->NextSiblingElement("trkseg"))
    {
      double segment_distance = 0.0;

      // std::println("Iterate trkpt...");
      tinyxml2::XMLElement *prev_trkpt = nullptr;
      // Iterate over each <trkpt> in the segment
      for (auto trkpt = segment->FirstChildElement("trkpt"); trkpt; trkpt = trkpt->NextSiblingElement("trkpt"))
      {
        if (prev_trkpt)
        {
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

  fclose(xmlFile);

  return total_distance;
}

double pugixml_gpx_length(const std::filesystem::path& path)
{
  pugi::xml_document doc;

  // Load the GPX file
  pugi::xml_parse_result result = doc.load_file(path.string().c_str());
  if (!result)
  {
    // std::cerr << "Failed to load GPX file: " << result.description() << "\n";
    return 0.0;
  }

  pugi::xml_node root = doc.child("gpx");

  double total_distance = 0.0;

  // Iterate over each <trk> element
  for (pugi::xml_node track = root.child("trk"); track; track = track.next_sibling("trk"))
  {
    // Iterate over each <trkseg> element
    for (pugi::xml_node segment = track.child("trkseg"); segment; segment = segment.next_sibling("trkseg"))
    {
      double segment_distance = 0.0;

      pugi::xml_node prev_trkpt;
      // Iterate over each <trkpt> in the segment
      for (pugi::xml_node trkpt = segment.child("trkpt"); trkpt; trkpt = trkpt.next_sibling("trkpt"))
      {
        if (prev_trkpt)
        {
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
