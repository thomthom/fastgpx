#include <string>
#include <print>

#include <pybind11/pybind11.h>

#include "fastgpx.hpp"
#include "gpx.hpp"

std::string process_string(const std::string &input)
{
    return "Processed: " + input;
}

double wrap_tinyxml_gpx_length(const std::string &gpx_path)
{
    // const auto length = tinyxml_gpx_length(gpx_data);
    // std::println("tinyxml path: {}", gpx_data);
    // std::filesystem::path path(gpx_data);
    // std::println("tinyxml path: {}", path);
    const auto length = tinyxml_gpx_length(gpx_path);
    return length;
}

double wrap_pugixml_gpx_length(const std::string &gpx_path)
{
    const auto length = pugixml_gpx_length(gpx_path);
    return length;
}

double wrap_fastgpx_gpx_length(const std::string &gpx_path)
{
    const auto gpx = fastgpx::ParseGpx(gpx_path);
    const auto length = gpx.GetLength2D();
    return length;
}

double wrap_fastgpx_track_length(const std::string &gpx_path, size_t tid)
{
    const auto gpx = fastgpx::ParseGpx(gpx_path);
    const auto track = gpx.tracks[tid];
    const auto length = track.GetLength2D();
    return length;
}

double wrap_fastgpx_segment_length(const std::string &gpx_path, size_t tid, size_t sid)
{
    const auto gpx = fastgpx::ParseGpx(gpx_path);
    const auto track = gpx.tracks[tid];
    const auto segment = track.segments[sid];
    const auto length = segment.GetLength2D();
    return length;
}

PYBIND11_MODULE(gpxcpp, m)
{
    m.def("process_string", &process_string,
          "A function that processes a string");

    m.def("tinyxml_gpx_length", &wrap_tinyxml_gpx_length,
          "Compute the length of the GPX file using tinyxml.");

    m.def("pugixml_gpx_length", &wrap_pugixml_gpx_length,
          "Compute the length of the GPX file using pugixml.");

    // fastgpx

    m.def("fastgpx_gpx_length", &wrap_fastgpx_gpx_length,
          "Compute the length of the GPX file using fastgpx.");

    m.def("fastgpx_track_length", &wrap_fastgpx_track_length,
          "Compute the length of the track using fastgpx.");

    m.def("fastgpx_segment_length", &wrap_fastgpx_segment_length,
          "Compute the length of the segment using fastgpx.");
}
