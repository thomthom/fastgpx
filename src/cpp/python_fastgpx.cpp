#include <filesystem>
#include <limits>
#include <stdexcept>

// #include <pybind11/chrono.h>
// #include <pybind11/functional.h>
// #include <pybind11/pybind11.h>
// #include <pybind11/stl.h>
// #include <pybind11/stl/filesystem.h>
// #include <pybind11/stl_bind.h>

#include <nanobind/nanobind.h>
#include <nanobind/stl/chrono.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>

#include "fastgpx/fastgpx.hpp"
#include "fastgpx/geom.hpp"
#include "fastgpx/polyline.hpp"

// #include "python_utc_chrono_nanobind.hpp"

namespace nb = nanobind;

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

NB_MODULE(fastgpx, m)
{
  nb::class_<fastgpx::TimeBounds>(m, "TimeBounds")
      .def(nb::init<>())
      .def(nb::init<OptionalTimePoint, OptionalTimePoint>(), nb::arg("start_time").none(),
           nb::arg("end_time").none())
      .def_rw("start_time", &fastgpx::TimeBounds::start_time, nb::arg("start_time").none())
      .def_rw("end_time", &fastgpx::TimeBounds::end_time, nb::arg("end_time").none())
      .def("is_empty", &fastgpx::TimeBounds::IsEmpty)
      .def("is_range", &fastgpx::TimeBounds::IsRange)
      .def("add",
           nb::overload_cast<std::chrono::system_clock::time_point>(&fastgpx::TimeBounds::Add),
           nb::arg("datetime"))
      .def("add", nb::overload_cast<const fastgpx::TimeBounds&>(&fastgpx::TimeBounds::Add),
           nb::arg("timebounds"));

  nb::class_<fastgpx::LatLong>(m, "LatLong")
      .def(nb::init<>())
      .def(nb::init<double, double, double>(), nb::arg("latitude"), nb::arg("longitude"),
           nb::arg("elevation") = 0.0)
      .def_rw("latitude", &fastgpx::LatLong::latitude)
      .def_rw("longitude", &fastgpx::LatLong::longitude)
      .def_rw("elevation", &fastgpx::LatLong::elevation);

  nb::class_<fastgpx::Bounds>(m, "Bounds")
      .def(nb::init<>())
      .def(nb::init<const fastgpx::LatLong&, const fastgpx::LatLong&>(), nb::arg("min"),
           nb::arg("max"))
      // Allow tuples instead of explicit LatLong objects.
      .def(
          "__init__",
          [](fastgpx::Bounds* obj, std::tuple<double, double> min_tuple,
             std::tuple<double, double> max_tuple) {
            fastgpx::LatLong min{std::get<0>(min_tuple), std::get<1>(min_tuple)};
            fastgpx::LatLong max{std::get<0>(max_tuple), std::get<1>(max_tuple)};
            return new (obj) fastgpx::Bounds(min, max);
          },
          nb::arg("min"), nb::arg("max"))
      .def_rw("min", &fastgpx::Bounds::min)
      .def_rw("max", &fastgpx::Bounds::max)
      .def("is_empty", &fastgpx::Bounds::IsEmpty)
      .def("add", nb::overload_cast<const fastgpx::LatLong&>(&fastgpx::Bounds::Add),
           nb::arg("location"))
      .def("add", nb::overload_cast<const fastgpx::Bounds&>(&fastgpx::Bounds::Add),
           nb::arg("bounds"))
      .def("max_bounds", &fastgpx::Bounds::MaxBounds, nb::arg("bounds"))
      // gpxpy compatibility:
      .def_prop_rw(
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
      .def_prop_rw(
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
      .def_prop_rw(
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

      .def_prop_rw(
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

  nb::class_<fastgpx::Segment>(m, "Segment")
      .def(nb::init<>()) // Default constructor
      .def_rw("points", &fastgpx::Segment::points)
      .def("bounds", &fastgpx::Segment::GetBounds)
      .def("get_bounds", &fastgpx::Segment::GetBounds) // gpxpy compatiblity
      .def("time_bounds", &fastgpx::Segment::GetTimeBounds)
      .def("get_time_bounds", &fastgpx::Segment::GetTimeBounds) // gpxpy compatiblity
      .def("length_2d", &fastgpx::Segment::GetLength2D)
      .def("length_3d", &fastgpx::Segment::GetLength3D);

  nb::class_<fastgpx::Track>(m, "Track")
      .def(nb::init<>()) // Default constructor
      .def_rw("name", &fastgpx::Track::name)
      .def_rw("comment", &fastgpx::Track::comment)
      .def_rw("description", &fastgpx::Track::description)
      .def_rw("number", &fastgpx::Track::number)
      .def_rw("type", &fastgpx::Track::type)
      .def_rw("segments", &fastgpx::Track::segments)
      .def("bounds", &fastgpx::Track::GetBounds)
      .def("get_bounds", &fastgpx::Track::GetBounds) // gpxpy compatiblity
      .def("time_bounds", &fastgpx::Track::GetTimeBounds)
      .def("get_time_bounds", &fastgpx::Track::GetTimeBounds) // gpxpy compatiblity
      .def("length_2d", &fastgpx::Track::GetLength2D)
      .def("length_3d", &fastgpx::Track::GetLength3D);

  nb::class_<fastgpx::Gpx>(m, "Gpx")
      .def(nb::init<>()) // Default constructor
      .def_rw("tracks", &fastgpx::Gpx::tracks)
      .def_rw("name", &fastgpx::Gpx::name)
      .def("bounds", &fastgpx::Gpx::GetBounds)
      .def("get_bounds", &fastgpx::Gpx::GetBounds) // gpxpy compatiblity
      .def("time_bounds", &fastgpx::Gpx::GetTimeBounds)
      .def("get_time_bounds", &fastgpx::Gpx::GetTimeBounds) // gpxpy compatiblity
      .def("length_2d", &fastgpx::Gpx::GetLength2D)
      .def("length_3d", &fastgpx::Gpx::GetLength3D);

  m.def("parse", nb::overload_cast<const std::string&>(&fastgpx::ParseGpx), nb::arg("path"));

  // fastgpx geo

  nb::module_ geo_mod = m.def_submodule("geo");

  geo_mod.def("haversine", &fastgpx::haversine, nb::arg("latlong1"), nb::arg("latlong2"));

  // Compatibility with gpxpy.geo.haversine_distance
  geo_mod
      .def(
          "haversine_distance",
          [](double latitude1, double longitude1, double latitude2, double longitude2) {
            const fastgpx::LatLong ll1(latitude1, longitude1);
            const fastgpx::LatLong ll2(latitude2, longitude2);
            return fastgpx::haversine(ll1, ll2);
          },
          nb::arg("latitude_1"), nb::arg("longitude_1"), nb::arg("latitude_2"),
          nb::arg("longitude_2"), "Compatibility with `gpxpy.geo.haversine_distance`")
      .doc() = "Algorithms for geographic calculations.";

  // fastgpx.polyline

  nb::module_ polyline_mod = m.def_submodule("polyline");

  nb::enum_<fastgpx::polyline::Precision>(polyline_mod, "Precision")
      .value("Five", fastgpx::polyline::Precision::Five)
      .value("Six", fastgpx::polyline::Precision::Six);

  polyline_mod.def(
      "encode",
      // Wrapping in std::vector because std::span doesn't work out of the box.
      [](const std::vector<fastgpx::LatLong>& points, fastgpx::polyline::Precision precision) {
        return fastgpx::polyline::encode(points, precision);
      },
      nb::arg("locations"), nb::arg("precision") = fastgpx::polyline::Precision::Five);

  polyline_mod.def(
      "encode",
      [](const std::vector<fastgpx::LatLong>& points, int precision) {
        return fastgpx::polyline::encode(points, IntToPrecision(precision));
      },
      nb::arg("locations"), nb::arg("precision") = 5);

  polyline_mod.def("decode", &fastgpx::polyline::decode, nb::arg("encoded"),
                   nb::arg("precision") = fastgpx::polyline::Precision::Five);

  polyline_mod.def(
      "decode",
      [](const std::string_view encoded, int precision) {
        return fastgpx::polyline::decode(encoded, IntToPrecision(precision));
      },
      nb::arg("locations"), nb::arg("precision") = 5);
}
