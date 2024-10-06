#include <string>
#include <print>

#include <pybind11/pybind11.h>

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

PYBIND11_MODULE(gpxcpp, m)
{
    m.def("process_string", &process_string,
          "A function that processes a string");

    m.def("tinyxml_gpx_length", &wrap_tinyxml_gpx_length,
          "Compute the length of the GPX file using tinyxml.");

    m.def("pugixml_gpx_length", &wrap_pugixml_gpx_length,
          "Compute the length of the GPX file using pugixml.");
}
