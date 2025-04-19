#include <filesystem>
#include <limits>
#include <stdexcept>

// #include <pybind11/chrono.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>
#include <pybind11/stl_bind.h>

#include "fastgpx/fastgpx.hpp"
#include "fastgpx/geom.hpp"
#include "fastgpx/polyline.hpp"

#include "python_utc_chrono.hpp"

namespace py = pybind11;

// TODO: def_readwrite for functions returning a container
// reference ends up returning a copy. Not ideal.
// https://pybind11.readthedocs.io/en/stable/advanced/cast/stl.html

namespace {

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

using OptionalTimePoint = std::optional<std::chrono::system_clock::time_point>;

} // namespace

PYBIND11_MODULE(fastgpx, m)
{
  py::class_<fastgpx::TimeBounds>(m, "TimeBounds")
      .def(py::init<>())
      .def(py::init<OptionalTimePoint, OptionalTimePoint>(), py::arg("start_time"),
           py::arg("end_time"))
      .def_readwrite("start_time", &fastgpx::TimeBounds::start_time)
      .def_readwrite("end_time", &fastgpx::TimeBounds::end_time)
      .def("is_empty", &fastgpx::TimeBounds::IsEmpty);

  py::class_<fastgpx::LatLong>(m, "LatLong")
      .def(py::init<>())
      .def(py::init<double, double, double>(), py::arg("latitude"), py::arg("longitude"),
           py::arg("elevation") = 0.0)
      .def_readwrite("latitude", &fastgpx::LatLong::latitude)
      .def_readwrite("longitude", &fastgpx::LatLong::longitude)
      .def_readwrite("elevation", &fastgpx::LatLong::elevation);

  py::class_<fastgpx::Bounds>(m, "Bounds")
      .def(py::init<>())
      .def(py::init<const fastgpx::LatLong&, const fastgpx::LatLong&>(), py::arg("min"),
           py::arg("max"))
      // Allow tuples instead of explicit LatLong objects.
      .def(py::init([](std::tuple<double, double> min_tuple, std::tuple<double, double> max_tuple) {
             fastgpx::LatLong min{std::get<0>(min_tuple), std::get<1>(min_tuple)};
             fastgpx::LatLong max{std::get<0>(max_tuple), std::get<1>(max_tuple)};
             return fastgpx::Bounds(min, max);
           }),
           py::arg("min"), py::arg("max"))
      .def_readwrite("min", &fastgpx::Bounds::min)
      .def_readwrite("max", &fastgpx::Bounds::max)
      .def("is_empty", &fastgpx::Bounds::IsEmpty)
      .def("add", py::overload_cast<const fastgpx::LatLong&>(&fastgpx::Bounds::Add),
           py::arg("location"))
      .def("add", py::overload_cast<const fastgpx::Bounds&>(&fastgpx::Bounds::Add),
           py::arg("bounds"))
      .def("max_bounds", &fastgpx::Bounds::MaxBounds, py::arg("bounds"))
      // gpxpy compatibility:
      .def_property(
          "min_latitude",
          [](const fastgpx::Bounds& self) -> std::optional<double> {
            if (self.min.has_value())
            {
              return self.min->latitude;
            }
            else
            {
              return std::nullopt;
            }
          },
          [](fastgpx::Bounds& self, double value) {
            if (!self.min.has_value())
            {
              // Kludge: Setting only one member is not ideal.
              self.min = {.latitude = std::numeric_limits<double>::max(),
                          .longitude = std::numeric_limits<double>::max()};
            }
            self.min->latitude = value;
          })
      .def_property(
          "min_longitude",
          [](const fastgpx::Bounds& self) -> std::optional<double> {
            if (self.min.has_value())
            {
              return self.min->longitude;
            }
            else
            {
              return std::nullopt;
            }
          },
          [](fastgpx::Bounds& self, double value) {
            if (!self.min.has_value())
            {
              // Kludge: Setting only one member is not ideal.
              self.min = {.latitude = std::numeric_limits<double>::max(),
                          .longitude = std::numeric_limits<double>::max()};
            }
            self.min->longitude = value;
          })
      .def_property(
          "max_latitude",
          [](const fastgpx::Bounds& self) -> std::optional<double> {
            if (self.max.has_value())
            {
              return self.max->latitude;
            }
            else
            {
              return std::nullopt;
            }
          },
          [](fastgpx::Bounds& self, double value) {
            if (!self.max.has_value())
            {
              // Kludge: Setting only one member is not ideal.
              self.max = {.latitude = std::numeric_limits<double>::min(),
                          .longitude = std::numeric_limits<double>::min()};
            }
            self.max->latitude = value;
          })

      .def_property(
          "max_longitude",
          [](const fastgpx::Bounds& self) -> std::optional<double> {
            if (self.max.has_value())
            {
              return self.max->longitude;
            }
            else
            {
              return std::nullopt;
            }
          },
          [](fastgpx::Bounds& self, double value) {
            if (!self.max.has_value())
            {
              // Kludge: Setting only one member is not ideal.
              self.max = {.latitude = std::numeric_limits<double>::min(),
                          .longitude = std::numeric_limits<double>::min()};
            }
            self.max->longitude = value;
          });

  py::class_<fastgpx::Segment>(m, "Segment")
      .def(py::init<>()) // Default constructor
      .def_readwrite("points", &fastgpx::Segment::points)
      .def("bounds", &fastgpx::Segment::GetBounds)
      .def("get_bounds", &fastgpx::Segment::GetBounds) // gpxpy compatiblity
      .def("time_bounds", &fastgpx::Segment::GetTimeBounds)
      .def("get_time_bounds", &fastgpx::Segment::GetTimeBounds) // gpxpy compatiblity
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
      .def("bounds", &fastgpx::Track::GetBounds)
      .def("get_bounds", &fastgpx::Track::GetBounds) // gpxpy compatiblity
      .def("time_bounds", &fastgpx::Track::GetTimeBounds)
      .def("get_time_bounds", &fastgpx::Track::GetTimeBounds) // gpxpy compatiblity
      .def("length_2d", &fastgpx::Track::GetLength2D)
      .def("length_3d", &fastgpx::Track::GetLength3D);

  py::class_<fastgpx::Gpx>(m, "Gpx")
      .def(py::init<>()) // Default constructor
      .def_readwrite("tracks", &fastgpx::Gpx::tracks)
      .def("bounds", &fastgpx::Gpx::GetBounds)
      .def("get_bounds", &fastgpx::Gpx::GetBounds) // gpxpy compatiblity
      .def("time_bounds", &fastgpx::Gpx::GetTimeBounds)
      .def("get_time_bounds", &fastgpx::Gpx::GetTimeBounds) // gpxpy compatiblity
      .def("length_2d", &fastgpx::Gpx::GetLength2D)
      .def("length_3d", &fastgpx::Gpx::GetLength3D);

  m.def("parse", py::overload_cast<const std::string&>(&fastgpx::ParseGpx), py::arg("path"));

  // fastgpx geo

  py::module_ geo_mod = m.def_submodule("geo");

  geo_mod.def("haversine", &fastgpx::haversine, py::arg("latlong1"), py::arg("latlong2"));

  // Compatibility with gpxpy.geo.haversine_distance
  geo_mod.def(
      "haversine_distance",
      [](double latitude1, double longitude1, double latitude2, double longitude2) {
        const fastgpx::LatLong ll1(latitude1, longitude1);
        const fastgpx::LatLong ll2(latitude2, longitude2);
        return fastgpx::haversine(ll1, ll2);
      },
      py::arg("latitude_1"), py::arg("longitude_1"), py::arg("latitude_2"), py::arg("longitude_2"),
      py::doc("Compatibility with `gpxpy.geo.haversine_distance`"));

  // fastgpx.polyline

  py::module_ polyline_mod = m.def_submodule("polyline");

  py::enum_<fastgpx::polyline::Precision>(polyline_mod, "Precision")
      .value("Five", fastgpx::polyline::Precision::Five)
      .value("Six", fastgpx::polyline::Precision::Six);

  polyline_mod.def(
      "encode",
      // Wrapping in std::vector because std::span doesn't work out of the box.
      [](const std::vector<fastgpx::LatLong>& points, fastgpx::polyline::Precision precision) {
        return fastgpx::polyline::encode(points, precision);
      },
      py::arg("locations"), py::arg("precision") = fastgpx::polyline::Precision::Five);

  polyline_mod.def(
      "encode",
      [](const std::vector<fastgpx::LatLong>& points, int precision) {
        return fastgpx::polyline::encode(points, IntToPrecision(precision));
      },
      py::arg("locations"), py::arg("precision") = 5);

  polyline_mod.def("decode", &fastgpx::polyline::decode, py::arg("encoded"),
                   py::arg("precision") = fastgpx::polyline::Precision::Five);

  polyline_mod.def(
      "decode",
      [](const std::string_view encoded, int precision) {
        return fastgpx::polyline::decode(encoded, IntToPrecision(precision));
      },
      py::arg("locations"), py::arg("precision") = 5);
}
