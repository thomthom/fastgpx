#include <filesystem>
#include <stdexcept>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/stl/filesystem.h>
#include <pybind11/functional.h>

#include "fastgpx.hpp"
#include "polyline.hpp"

namespace py = pybind11;

// TODO: def_readwrite for functions returning a container
// reference ends up returning a copy. Not ideal.
// https://pybind11.readthedocs.io/en/stable/advanced/cast/stl.html

namespace
{

  fastgpx::polyline::Precision IntToPrecision(const int value)
  {
    fastgpx::polyline::Precision precision;
    if (value == 6)
    {
      precision = fastgpx::polyline::Precision::Six;
    }
    else if (value == 5)
    {
      precision = fastgpx::polyline::Precision::Five;
    }
    else
    {
      throw std::invalid_argument("Invalid precision value. Must be 5 or 6.");
    }
    return precision;
  }

} // namespace

PYBIND11_MODULE(fastgpx, m)
{
  py::class_<fastgpx::LatLong>(m, "LatLong")
      .def(py::init<>()) // Default constructor
      .def_readwrite("latitude", &fastgpx::LatLong::latitude)
      .def_readwrite("longitude", &fastgpx::LatLong::longitude)
      .def_readwrite("elevation", &fastgpx::LatLong::elevation);

  py::class_<fastgpx::Segment>(m, "Segment")
      .def(py::init<>()) // Default constructor
      .def_readwrite("points", &fastgpx::Segment::points)
      .def("length_2d", &fastgpx::Segment::GetLength2D)
      .def("length_3d", &fastgpx::Segment::GetLength3D);

  py::class_<fastgpx::Track>(m, "Track")
      .def(py::init<>()) // Default constructor
      .def_readwrite("name", &fastgpx::Track::name)
      .def_readwrite("comment", &fastgpx::Track::comment)
      .def_readwrite("description", &fastgpx::Track::description)
      .def_readwrite("number", &fastgpx::Track::number)
      .def_readwrite("type", &fastgpx::Track::type)
      .def_readwrite("segments", &fastgpx::Track::segments)
      .def("length_2d", &fastgpx::Track::GetLength2D)
      .def("length_3d", &fastgpx::Track::GetLength3D);

  py::class_<fastgpx::Gpx>(m, "Gpx")
      .def(py::init<>()) // Default constructor
      .def_readwrite("tracks", &fastgpx::Gpx::tracks)
      .def("length_2d", &fastgpx::Gpx::GetLength2D)
      .def("length_3d", &fastgpx::Gpx::GetLength3D);

  m.def("parse", py::overload_cast<const std::string &>(&fastgpx::ParseGpx),
        py::arg("path"));

  py::module_ polyline_mod = m.def_submodule("polyline");

  py::enum_<fastgpx::polyline::Precision>(polyline_mod, "Precision")
      .value("Five", fastgpx::polyline::Precision::Five)
      .value("Six", fastgpx::polyline::Precision::Six);

  polyline_mod.def(
      "encode",
      // Wrapping in std::vector because std::span doesn't work out of the box.
      [](const std::vector<fastgpx::LatLong> &points, fastgpx::polyline::Precision precision)
      {
        return fastgpx::polyline::encode(points, precision);
      },
      py::arg("locations"),
      py::arg("precision") = fastgpx::polyline::Precision::Five);

  polyline_mod.def(
      "encode",
      [](const std::vector<fastgpx::LatLong> &points, int precision)
      {
        return fastgpx::polyline::encode(points, IntToPrecision(precision));
      },
      py::arg("locations"),
      py::arg("precision") = 5);

  polyline_mod.def(
      "decode",
      &fastgpx::polyline::decode,
      py::arg("encoded"),
      py::arg("precision") = fastgpx::polyline::Precision::Five);

  polyline_mod.def(
      "decode",
      [](const std::string_view encoded, int precision)
      {
        return fastgpx::polyline::decode(encoded, IntToPrecision(precision));
      },
      py::arg("locations"),
      py::arg("precision") = 5);
}
