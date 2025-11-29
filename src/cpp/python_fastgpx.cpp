#include <filesystem>
#include <format>
#include <limits>
#include <stdexcept>

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
// #include <nanobind/stl/chrono.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>

#include "fastgpx/fastgpx.hpp"
#include "fastgpx/geom.hpp"
#include "fastgpx/polyline.hpp"

#include "python_utc_chrono_nanobind.hpp"

using chrono_timepoint = std::chrono::system_clock::time_point;
using namespace fastgpx;
namespace nb = nanobind;
using namespace nb::literals;

namespace {

polyline::Precision IntToPrecision(const int value)
{
  polyline::Precision precision;
  if (value == 6)
  {
    precision = polyline::Precision::Six;
  }
  else if (value == 5)
  {
    precision = polyline::Precision::Five;
  }
  else
  {
    throw std::invalid_argument("Invalid precision value. Must be 5 or 6.");
  }
  return precision;
}

std::string FormatLatLongAsTuples(const LatLong& ll)
{
  return std::format("({}, {}, {})", ll.latitude, ll.longitude, ll.elevation);
}

std::string FormatLatLongAsTuples(const std::optional<LatLong>& ll)
{
  return ll.has_value() ? FormatLatLongAsTuples(*ll) : "None";
}

std::string FormatTimePointAsISO8601(const chrono_timepoint& tp)
{
  const auto tp_seconds = std::chrono::floor<std::chrono::seconds>(tp);
  return std::format("{:%Y-%m-%dT%H:%M:%SZ}", tp_seconds);
}

std::string FormatTimePointAsISO8601(const std::optional<chrono_timepoint>& tp)
{
  return tp.has_value() ? FormatTimePointAsISO8601(*tp) : "None";
}

// Format as a python datetime string
std::string FormatTimePointAsDateTime(const chrono_timepoint& tp)
{
  nb::object py = nb::cast(tp);
  return nb::repr(py).c_str();
}

std::string FormatTimePointAsDateTime(const std::optional<chrono_timepoint>& tp)
{
  return tp.has_value() ? FormatTimePointAsDateTime(*tp) : "None";
}

template<double LatLong::* Member>
std::optional<double> GetBoundsMember(const Bounds& self, std::optional<LatLong> Bounds::* field)
{
  if ((self.*field).has_value())
  {
    return (self.*field).value().*Member;
  }
  else
  {
    return std::nullopt;
  }
}

template<double LatLong::* Member>
void SetBoundsMember(Bounds& self, std::optional<LatLong> Bounds::* field, double value,
                     double default_value)
{
  if (!(self.*field).has_value())
  {
    // Kludge: Setting only one member is not ideal.
    self.*field = {.latitude = default_value, .longitude = default_value};
  }
  (self.*field).value().*Member = value;
}

} // namespace

NB_MODULE(fastgpx, m)
{
  nb::class_<TimeBounds>(m, "TimeBounds")
      .def(nb::init<>())
      .def(nb::init<std::optional<chrono_timepoint>, std::optional<chrono_timepoint>>(),
           "start_time"_a.none(), "end_time"_a.none())
      .def_rw("start_time", &TimeBounds::start_time, "start_time"_a.none())
      .def_rw("end_time", &TimeBounds::end_time, "end_time"_a.none())
      .def("is_empty", &TimeBounds::IsEmpty)
      .def("is_range", &TimeBounds::IsRange)
      .def("add", nb::overload_cast<chrono_timepoint>(&TimeBounds::Add), "datetime"_a)
      .def("add", nb::overload_cast<const TimeBounds&>(&TimeBounds::Add), "timebounds"_a)
      .def(nb::self == nb::self, nb::sig("def __eq__(self, arg: object, /) -> bool"))
      .def("__repr__",
           [](const TimeBounds& tb) {
             const auto start_time = FormatTimePointAsDateTime(tb.start_time);
             const auto end_time = FormatTimePointAsDateTime(tb.end_time);
             return std::format("fastgpx.TimeBounds(start_time={}, end_time={})", start_time,
                                end_time);
           })
      .def("__str__", [](const TimeBounds& tb) {
        const auto start_time = FormatTimePointAsISO8601(tb.start_time);
        const auto end_time = FormatTimePointAsISO8601(tb.end_time);
        return std::format("TimeBounds({} to {})", start_time, end_time);
      });

  nb::class_<LatLong>(m, "LatLong")
      .def(nb::init<>())
      .def(nb::init<double, double, double>(), "latitude"_a, "longitude"_a, "elevation"_a = 0.0)
      .def_rw("latitude", &LatLong::latitude,
              "The latitude of the point. Decimal degrees, WGS84 datum.")
      .def_rw("longitude", &LatLong::longitude,
              "The longitude of the point. Decimal degrees, WGS84 datum.")
      .def_rw("elevation", &LatLong::elevation, "The elevation of the point in meters.")
      // .def_rw("time", &LatLong::time,
      //         "Creation/modification timestamp for element. Date and time in are in Univeral "
      //         "Coordinated Time (UTC), not local time! Conforms to ISO 8601 specification for "
      //         "date/time representation. Fractional seconds are allowed for millisecond timing "
      //         "in tracklogs.")
      .def(nb::self == nb::self, nb::sig("def __eq__(self, arg: object, /) -> bool"))
      .def("__repr__",
           [](const LatLong& ll) {
             return std::format("fastgpx.LatLong(latitude={}, longitude={}, elevation={})",
                                ll.latitude, ll.longitude, ll.elevation);
           })
      .def("__str__",
           [](const LatLong& ll) {
             return std::format("LatLong({}, {}, {})", ll.latitude, ll.longitude, ll.elevation);
           })
      .doc() = "Represent ``<trkpt>`` data in GPX files.";

  nb::class_<Bounds>(m, "Bounds")
      .def(nb::init<>())
      .def(nb::init<const LatLong&, const LatLong&>(), "min"_a, "max"_a)
      // Allow tuples instead of explicit LatLong objects.
      .def(
          "__init__",
          [](Bounds* obj, std::tuple<double, double> min_tuple,
             std::tuple<double, double> max_tuple) {
            LatLong min{std::get<0>(min_tuple), std::get<1>(min_tuple)};
            LatLong max{std::get<0>(max_tuple), std::get<1>(max_tuple)};
            new (obj) Bounds(min, max);
          },
          "min"_a, "max"_a)
      .def(
          "__init__",
          [](Bounds* obj, std::tuple<double, double, double> min_tuple,
             std::tuple<double, double, double> max_tuple) {
            LatLong min{std::get<0>(min_tuple), std::get<1>(min_tuple), std::get<2>(min_tuple)};
            LatLong max{std::get<0>(max_tuple), std::get<1>(max_tuple), std::get<2>(max_tuple)};
            new (obj) Bounds(min, max);
          },
          "min"_a, "max"_a)
      .def_rw("min", &Bounds::min)
      .def_rw("max", &Bounds::max)
      .def("is_empty", &Bounds::IsEmpty)
      .def("add", nb::overload_cast<const LatLong&>(&Bounds::Add), "location"_a)
      .def("add", nb::overload_cast<const Bounds&>(&Bounds::Add), "bounds"_a)
      .def("max_bounds", &Bounds::MaxBounds, "bounds"_a)
      // gpxpy compatibility:
      .def_prop_rw(
          "min_latitude",
          [](const Bounds& self) {
            return GetBoundsMember<&LatLong::latitude>(self, &Bounds::min);
          },
          [](Bounds& self, double value) {
            SetBoundsMember<&LatLong::latitude>(self, &Bounds::min, value,
                                                std::numeric_limits<double>::max());
          },
          ".. warning::\n\n"
          "   Compatibility with ``gpxpy.GPXBounds.min_latitude``.\n"
          "   Prefer ``min.latitude`` instead. :func:`min`\n")
      .def_prop_rw(
          "min_longitude",
          [](const Bounds& self) {
            return GetBoundsMember<&LatLong::longitude>(self, &Bounds::min);
          },
          [](Bounds& self, double value) {
            SetBoundsMember<&LatLong::longitude>(self, &Bounds::min, value,
                                                 std::numeric_limits<double>::max());
          },
          ".. warning::\n\n"
          "   Compatibility with ``gpxpy.GPXBounds.min_longitude``.\n"
          "   Prefer ``min.longitude`` instead. :func:`min`\n")
      .def_prop_rw(
          "max_latitude",
          [](const Bounds& self) {
            return GetBoundsMember<&LatLong::latitude>(self, &Bounds::max);
          },
          [](Bounds& self, double value) {
            SetBoundsMember<&LatLong::latitude>(self, &Bounds::max, value,
                                                std::numeric_limits<double>::min());
          },
          ".. warning::\n\n"
          "   Compatibility with ``gpxpy.GPXBounds.max_latitude``.\n"
          "   Prefer ``max.latitude`` instead. :func:`max`\n")
      .def_prop_rw(
          "max_longitude",
          [](const Bounds& self) {
            return GetBoundsMember<&LatLong::longitude>(self, &Bounds::max);
          },
          [](Bounds& self, double value) {
            SetBoundsMember<&LatLong::longitude>(self, &Bounds::max, value,
                                                 std::numeric_limits<double>::min());
          },
          ".. warning::\n\n"
          "   Compatibility with ``gpxpy.GPXBounds.max_longitude``.\n"
          "   Prefer ``max.longitude`` instead. :func:`max`\n")
      .def(nb::self == nb::self, nb::sig("def __eq__(self, arg: object, /) -> bool"))
      .def("__repr__",
           [](const Bounds& ll) {
             const auto min = FormatLatLongAsTuples(ll.min);
             const auto max = FormatLatLongAsTuples(ll.max);
             return std::format("fastgpx.Bounds(min={}, max={})", min, max);
           })
      .def("__str__", [](const Bounds& ll) {
        const auto min = FormatLatLongAsTuples(ll.min);
        const auto max = FormatLatLongAsTuples(ll.max);
        return std::format("Bounds(min={}, max={})", min, max);
      });

  nb::class_<Segment>(m, "Segment")
      .def(nb::init<>()) // Default constructor
      .def_rw("points", &Segment::points)
      .def("bounds", &Segment::GetBounds)
      .def("get_bounds", &Segment::GetBounds,
           ".. warning::\n\n"
           "   Compatibility with ``gpxpy.GPXTrackSegment.get_bounds``.\n"
           "   Prefer :func:`bounds` instead.\n") // gpxpy compatiblity
      .def("time_bounds", &Segment::GetTimeBounds)
      .def("get_time_bounds", &Segment::GetTimeBounds,
           ".. warning::\n\n"
           "   Compatibility with ``gpxpy.GPXTrackSegment.get_time_bounds``.\n"
           "   Prefer :func:`time_bounds` instead.\n") // gpxpy compatiblity
      .def("length_2d", &Segment::GetLength2D, "Distance in meters.")
      .def("length_3d", &Segment::GetLength3D, "Distance in meters.")
      .def("__repr__",
           [](const Segment& s) {
             return std::format("<fastgpx.Segment(points: {})>", s.points.size());
           })
      .doc() = "Represent ``<trkseg>`` data in GPX files.";

  nb::class_<Track>(m, "Track")
      .def(nb::init<>()) // Default constructor
      .def_rw("name", &Track::name)
      .def_rw("comment", &Track::comment)
      .def_rw("description", &Track::description)
      .def_rw("number", &Track::number)
      .def_rw("type", &Track::type)
      .def_rw("segments", &Track::segments)
      .def("bounds", &Track::GetBounds)
      .def("get_bounds", &Track::GetBounds,
           ".. warning::\n\n"
           "   Compatibility with ``gpxpy.GPXTrack.get_bounds``.\n"
           "   Prefer :func:`bounds` instead.\n") // gpxpy compatiblity
      .def("time_bounds", &Track::GetTimeBounds)
      .def("get_time_bounds", &Track::GetTimeBounds,
           ".. warning::\n\n"
           "   Compatibility with ``gpxpy.GPXTrack.get_time_bounds``.\n"
           "   Prefer :func:`time_bounds` instead.\n") // gpxpy compatiblity
      .def("length_2d", &Track::GetLength2D, "Distance in meters.")
      .def("length_3d", &Track::GetLength3D, "Distance in meters.")
      .def("__repr__",
           [](const Track& t) {
             return std::format("<fastgpx.Track(segments: {})>", t.segments.size());
           })
      .doc() = "Represent ``<trk>`` data in GPX files.";

  nb::class_<Gpx>(m, "Gpx")
      .def(nb::init<>()) // Default constructor
      .def_rw("tracks", &Gpx::tracks)
      .def_rw("name", &Gpx::name)
      .def("bounds", &Gpx::GetBounds)
      .def("get_bounds", &Gpx::GetBounds,
           ".. warning::\n\n"
           "   Compatibility with ``gpxpy.GPX.get_bounds``.\n"
           "   Prefer :func:`bounds` instead.\n") // gpxpy compatiblity
      .def("time_bounds", &Gpx::GetTimeBounds)
      .def("get_time_bounds", &Gpx::GetTimeBounds,
           ".. warning::\n\n"
           "   Compatibility with ``gpxpy.GPX.get_time_bounds``.\n"
           "   Prefer :func:`time_bounds` instead.\n") // gpxpy compatiblity
      .def("length_2d", &Gpx::GetLength2D, "Distance in meters.")
      .def("length_3d", &Gpx::GetLength3D, "Distance in meters.")
      .def("__repr__",
           [](const Gpx& g) {
             if (g.name.has_value())
             {
               return std::format("<fastgpx.Gpx(tracks: {}, name: '{}')>", //
                                  g.tracks.size(), *g.name);
             }
             return std::format("<fastgpx.Gpx(tracks: {})>", g.tracks.size());
           })
      .doc() = "Represent ``<gpx>`` data in GPX files.";

  m.def("parse", nb::overload_cast<const std::string&>(&ParseGpx), "path"_a);

  // fastgpx geo

  nb::module_ geo_mod = m.def_submodule("geo");

  geo_mod.def("haversine", &haversine, "latlong1"_a, "latlong2"_a,
              "Haversine distance returned in meters using ``osmium`` logic.");

  // Signature compatibility with gpxpy.geo.haversine_distance
  geo_mod
      .def(
          "haversine_distance",
          [](double latitude1, double longitude1, double latitude2, double longitude2) {
            const LatLong ll1(latitude1, longitude1);
            const LatLong ll2(latitude2, longitude2);
            return haversine(ll1, ll2);
          },
          "latitude_1"_a, "longitude_1"_a, "latitude_2"_a, "longitude_2"_a,
          ".. warning::\n\n"
          "   Signature compatibility with ``gpxpy.geo.haversine_distance``.\n"
          "   Prefer :func:`haversine` instead.\n\n"
          ".. important::\n\n"
          "   While the signature matches ``gpxpy``, the implementation uses ``osmium`` logic "
          "   which may lead to slightly different results.")

      .doc() = "Algorithms for geographic calculations.";

  // fastgpx.polyline

  nb::module_ polyline_mod = m.def_submodule("polyline");

  nb::enum_<polyline::Precision>(polyline_mod, "Precision")
      .value("Five", polyline::Precision::Five)
      .value("Six", polyline::Precision::Six);

  polyline_mod.def(
      "encode",
      // Wrapping in std::vector because std::span doesn't work out of the box.
      [](const std::vector<LatLong>& points, polyline::Precision precision) {
        return polyline::encode(points, precision);
      },
      "locations"_a, "precision"_a = polyline::Precision::Five);

  polyline_mod.def(
      "encode",
      [](const std::vector<LatLong>& points, int precision) {
        return polyline::encode(points, IntToPrecision(precision));
      },
      "locations"_a, "precision"_a = 5);

  polyline_mod.def("decode", &polyline::decode, "encoded"_a,
                   "precision"_a = polyline::Precision::Five);

  polyline_mod.def(
      "decode",
      [](const std::string_view encoded, int precision) {
        return polyline::decode(encoded, IntToPrecision(precision));
      },
      "locations"_a, "precision"_a = 5);
}
