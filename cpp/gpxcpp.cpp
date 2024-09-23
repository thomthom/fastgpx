#include <string>

#include <pybind11/pybind11.h>

std::string process_string(const std::string& input) {
    return "Processed: " + input;
}

PYBIND11_MODULE(gpxcpp, m) {
    m.def("process_string", &process_string, "A function that processes a string");
}
