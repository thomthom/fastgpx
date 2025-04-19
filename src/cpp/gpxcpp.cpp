#include <print>
#include <string>

#include <pybind11/pybind11.h>

#include "fastgpx/fastgpx.hpp"
#include "gpx.hpp"

std::string process_string(const std::string& input)
{
  return "Processed: " + input;
}

// tinyxml

double wrap_tinyxml_gpx_length2d(const std::string& gpx_path)
{
  const auto length = tinyxml_gpx_length2d(gpx_path);
  return length;
}

// pugixml

double wrap_pugixml_gpx_length2d(const std::string& gpx_path)
{
  const auto length = pugixml_gpx_length2d(gpx_path);
  return length;
}

// fastgpx

double wrap_fastgpx_gpx_length2d(const std::string& gpx_path)
{
  const auto gpx = fastgpx::ParseGpx(gpx_path);
  const auto length = gpx.GetLength2D();
  return length;
}

double wrap_fastgpx_track_length(const std::string& gpx_path, size_t tid)
{
  const auto gpx = fastgpx::ParseGpx(gpx_path);
  const auto track = gpx.tracks[tid];
  const auto length = track.GetLength2D();
  return length;
}

double wrap_fastgpx_segment_length(const std::string& gpx_path, size_t tid, size_t sid)
{
  const auto gpx = fastgpx::ParseGpx(gpx_path);
  const auto track = gpx.tracks[tid];
  const auto segment = track.segments[sid];
  const auto length = segment.GetLength2D();
  return length;
}

PYBIND11_MODULE(gpxcpp, m)
{
  m.def("process_string", &process_string, "A function that processes a string");

  // tinyxml

  m.def("tinyxml_gpx_length2d", &wrap_tinyxml_gpx_length2d,
        "Compute the length of the GPX file using tinyxml.");

  // pugixml

  m.def("pugixml_gpx_length2d", &wrap_pugixml_gpx_length2d,
        "Compute the length of the GPX file using pugixml.");

  // fastgpx

  m.def("fastgpx_gpx_length2d", &wrap_fastgpx_gpx_length2d,
        "Compute the length of the GPX file using fastgpx.");

  m.def("fastgpx_track_length2d", &wrap_fastgpx_track_length,
        "Compute the length of the track using fastgpx.");

  m.def("fastgpx_segment_length2d", &wrap_fastgpx_segment_length,
        "Compute the length of the segment using fastgpx.");
}
