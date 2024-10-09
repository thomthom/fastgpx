#include "fastgpx.hpp"

#include <filesystem>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/stl/filesystem.h>
#include <pybind11/functional.h>

namespace py = pybind11;

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

  m.def("parse", &fastgpx::ParseGpx, py::arg("path"));
}
