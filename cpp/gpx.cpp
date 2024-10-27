#include "gpx.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <numbers>
#include <print>

#include <tinyxml2.h>

#include <pugixml.hpp>

#include "fastgpx.hpp"
#include "filesystem.hpp"
#include "geom.hpp"

double tinyxml_gpx_length2d(const std::filesystem::path &path)
{
  FILE *xmlFile = fastgpx::open_file(path);

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
    for (auto segment = track->FirstChildElement("trkseg"); segment;
         segment = segment->NextSiblingElement("trkseg"))
    {
      double segment_distance = 0.0;

      // std::println("Iterate trkpt...");
      tinyxml2::XMLElement *prev_trkpt = nullptr;
      // Iterate over each <trkpt> in the segment
      for (auto trkpt = segment->FirstChildElement("trkpt"); trkpt;
           trkpt = trkpt->NextSiblingElement("trkpt"))
      {
        if (prev_trkpt)
        {
          // Get current trackpoint latitude and longitude
          double lat1 = std::stod(prev_trkpt->Attribute("lat"));
          double lon1 = std::stod(prev_trkpt->Attribute("lon"));
          double lat2 = std::stod(trkpt->Attribute("lat"));
          double lon2 = std::stod(trkpt->Attribute("lon"));

          // Compute the distance between two track points
          segment_distance += fastgpx::v1::distance2d({lat1, lon1}, {lat2, lon2});
        }
        prev_trkpt = trkpt;
      }
      total_distance += segment_distance;
    }
  }

  fclose(xmlFile);

  return total_distance;
}

double pugixml_gpx_length2d(const std::filesystem::path &path)
{
  pugi::xml_document doc;

  // Load the GPX file
  const auto path16 = fastgpx::utf8_to_utf16(path.string());
  pugi::xml_parse_result result = doc.load_file(path16.c_str());
  // pugi::xml_parse_result result = doc.load_file(path.string().c_str());
  if (!result)
  {
    // std::cerr << "Failed to load GPX file: " << result.description() << "\n";
    std::println("Failed to load GPX file: {} - {}", result.description(), path.string());
    return 0.0;
  }

  pugi::xml_node root = doc.child("gpx");

  double total_distance = 0.0;

  // Iterate over each <trk> element
  for (pugi::xml_node track = root.child("trk"); track; track = track.next_sibling("trk"))
  {
    // Iterate over each <trkseg> element
    for (pugi::xml_node segment = track.child("trkseg"); segment;
         segment = segment.next_sibling("trkseg"))
    {
      double segment_distance = 0.0;

      pugi::xml_node prev_trkpt;
      // Iterate over each <trkpt> in the segment
      for (pugi::xml_node trkpt = segment.child("trkpt"); trkpt;
           trkpt = trkpt.next_sibling("trkpt"))
      {
        if (prev_trkpt)
        {
          // Get current and previous trackpoint lat/lon attributes
          double lat1 = prev_trkpt.attribute("lat").as_double();
          double lon1 = prev_trkpt.attribute("lon").as_double();
          double lat2 = trkpt.attribute("lat").as_double();
          double lon2 = trkpt.attribute("lon").as_double();

          // Compute distance between two points
          segment_distance += fastgpx::v1::distance2d({lat1, lon1}, {lat2, lon2});
        }
        prev_trkpt = trkpt;
      }
      total_distance += segment_distance;
    }
  }

  return total_distance;
}
