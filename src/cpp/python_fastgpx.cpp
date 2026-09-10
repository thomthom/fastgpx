#include <algorithm>
#include <filesystem>
#include <format>
#include <limits>
#include <stdexcept>
#include <utility>

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
// #include <nanobind/stl/chrono.h>
#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/tuple.h>
// Deliberately not <nanobind/stl/vector.h>: its list caster would take precedence over the bound
// container types below for every std::vector in this file. See the container section in
// NB_MODULE.

#include "fastgpx/fastgpx.hpp"
#include "fastgpx/geom.hpp"
#include "fastgpx/errors.hpp"
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

// Clamp a Python slice-style [start, stop) pair to a vector of `size` elements, the way
// `list.index` does: negative values count from the end, and out-of-range values are clamped.
std::pair<size_t, size_t> ClampRange(Py_ssize_t start, std::optional<Py_ssize_t> stop_arg,
                                     size_t size)
{
  const auto n = static_cast<Py_ssize_t>(size);
  Py_ssize_t stop = stop_arg.value_or(n);
  if (start < 0)
  {
    start = std::max<Py_ssize_t>(start + n, 0);
  }
  if (stop < 0)
  {
    stop += n;
  }
  stop = std::clamp<Py_ssize_t>(stop, 0, n);
  start = std::min(start, stop);
  return {static_cast<size_t>(start), static_cast<size_t>(stop)};
}

// Register a bound class with a `collections.abc` ABC so that `isinstance` agrees with the base
// declared in the type stub. The ABC's mixin methods are not inherited by registration, so every
// method the ABC promises has to be bound explicitly.
void RegisterAbc(nb::handle cls, const char* abc)
{
  nb::module_::import_("collections.abc").attr(abc).attr("register")(cls);
}

// Bind `Vector` as a read-only `collections.abc.Sequence` whose elements are references into the
// vector. Each element wrapper keeps the sequence alive, and the sequence keeps the object it was
// read from alive, so an element outlives its document safely. Nothing here can add, remove or
// reorder elements: that would move or destroy the objects other wrappers still point at. The
// mutable container uses nb::bind_vector with copy semantics instead; see the comment in
// NB_MODULE. The element types have no equality, so `in`, `count` and `index` compare identity,
// which is also what Python's default `==` does for them.
template<typename Vector>
nb::class_<Vector> BindReadOnlySequence(nb::handle scope, const char* name, const char* signature)
{
  using Value = typename Vector::value_type;
  auto find = [](const Vector& v, const Value& value, Py_ssize_t start,
                 std::optional<Py_ssize_t> stop) {
    const auto [first, last] = ClampRange(start, stop, v.size());
    for (size_t i = first; i < last; ++i)
    {
      if (&v[i] == &value)
      {
        return static_cast<Py_ssize_t>(i);
      }
    }
    return Py_ssize_t{-1};
  };
  auto cls =
      nb::class_<Vector>(scope, name, nb::sig(signature))
          .def("__len__", [](const Vector& v) { return v.size(); })
          .def("__bool__", [](const Vector& v) { return !v.empty(); })
          .def(
              "__getitem__",
              [](Vector& v, Py_ssize_t index) -> Value& {
                const auto size = static_cast<Py_ssize_t>(v.size());
                if (index < 0)
                {
                  index += size;
                }
                if (index < 0 || index >= size)
                {
                  throw nb::index_error("index out of range");
                }
                return v[static_cast<size_t>(index)];
              },
              nb::rv_policy::reference_internal, "index"_a)
          // A slice is a Python list of the same reference elements, each keeping the
          // sequence alive as an indexed element would.
          .def(
              "__getitem__",
              [](nb::handle_t<Vector> self, const nb::slice& slice) {
                const Vector& v = nb::cast<const Vector&>(self);
                auto [start, stop, step, length] = slice.compute(v.size());
                nb::list result;
                for (size_t i = 0; i < length; ++i)
                {
                  result.append(nb::cast(v[static_cast<size_t>(start)],
                                         nb::rv_policy::reference_internal, self));
                  start += step;
                }
                return nb::typed<nb::list, Value>(std::move(result));
              },
              "index"_a)
          // The iterator is index based, as bind_vector's is, and reads the live vector on
          // every step, so it holds no C++ iterator that a resize could invalidate.
          .def("__iter__",
               [](nb::handle_t<Vector> self) {
                 return nb::detail::make_index_iterator<nb::rv_policy::reference_internal, Vector>(
                     nb::type<Vector>(), "Iterator", self);
               })
          // A reversed slice is a list of reference wrappers taken up front.
          .def("__reversed__",
               [](nb::handle_t<Vector> self) {
                 return nb::typed<nb::iterator, Value>(
                     nb::iter(self[nb::slice(nb::none(), nb::none(), nb::int_(-1))]));
               })
          .def(
              "__contains__",
              [find](const Vector& v, const Value& value) {
                return find(v, value, 0, std::nullopt) >= 0;
              },
              "value"_a)
          // Fallback for values of another type, which are never contained.
          .def(
              "__contains__", [](const Vector&, nb::handle) { return false; }, "value"_a)
          .def(
              "count",
              [find](const Vector& v, const Value& value) {
                return find(v, value, 0, std::nullopt) >= 0 ? 1 : 0;
              },
              "value"_a)
          .def(
              "count", [](const Vector&, nb::handle) { return 0; }, "value"_a)
          .def(
              "index",
              [find](const Vector& v, const Value& value, Py_ssize_t start,
                     std::optional<Py_ssize_t> stop) {
                const auto index = find(v, value, start, stop);
                if (index < 0)
                {
                  throw nb::value_error("value is not in the sequence");
                }
                return index;
              },
              "value"_a, "start"_a = 0, "stop"_a.none() = nb::none())
          .def(
              "index",
              [](const Vector&, nb::handle, Py_ssize_t, std::optional<Py_ssize_t>) -> Py_ssize_t {
                throw nb::value_error("value is not in the sequence");
              },
              "value"_a, "start"_a = 0, "stop"_a.none() = nb::none())
          .def("__repr__", [name](const Vector& v) {
            return std::format("<fastgpx.{}(items: {})>", name, v.size());
          });
  RegisterAbc(cls, "Sequence");
  return cls;
}

// Build a Python list of LatLong copies from `points` in one pass. Sized up front and filled in
// place, as nanobind's own list caster does. PyList_SetItem is the stable ABI spelling; it steals
// the reference and cannot fail on an index in range. A list whose remaining slots are still NULL
// is safe to destroy if a cast throws part way through.
nb::typed<nb::list, LatLong> LatLongsToList(const std::vector<LatLong>& points)
{
  const auto size = static_cast<Py_ssize_t>(points.size());
  nb::list result = nb::steal<nb::list>(nb::detail::raise_if_null(PyList_New(size)));
  for (Py_ssize_t i = 0; i < size; ++i)
  {
    PyList_SetItem(result.ptr(), i, nb::cast(points[static_cast<size_t>(i)]).release().ptr());
  }
  return nb::typed<nb::list, LatLong>(std::move(result));
}

} // namespace

NB_MODULE(fastgpx, m)
{
  // Exceptions
  //
  // `fastgpx_error` derives from `std::runtime_error`, which nanobind would map to a bare
  // `RuntimeError`. Every fastgpx error describes input the library cannot use, so the hierarchy
  // is exposed as `ValueError` subclasses instead: `fastgpx.Error` for the base (which also covers
  // `value_error`) and `fastgpx.ParseError` for malformed GPX, polyline or timestamp data.
  // Translators are tried most-recently-registered first, so the base goes first.
  nb::exception<fastgpx_error> error_type(m, "Error", PyExc_ValueError);
  error_type.attr("__doc__") =
      "Base class for everything fastgpx raises about its input. A ``ValueError``.";
  nb::exception<parse_error> parse_error_type(m, "ParseError", error_type.ptr());
  parse_error_type.attr("__doc__") =
      "Malformed GPX data, polyline string or timestamp.\n\n"
      "Timestamps are parsed on demand, so a malformed ``<time>`` raises from "
      "``time_bounds()`` rather than from ``load()`` or ``parse()``.";

  // File access failures are `OSError`s in Python, not value errors.
  nb::register_exception_translator([](const std::exception_ptr& p, void*) {
    try
    {
      std::rethrow_exception(p);
    }
    catch (const fastgpx::file_error& e)
    {
      PyErr_SetString(e.not_found() ? PyExc_FileNotFoundError : PyExc_OSError, e.what());
    }
  });

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

  // Containers
  //
  // `Segment::points`, `Track::segments` and `Gpx::tracks` are exposed as the C++ vectors rather
  // than converted to a Python list on every attribute access. The list conversion made `len()`
  // and indexing O(n) per access, silently discarded `append()`, and handed out element wrappers
  // that did not keep the document alive.
  //
  // `LatLongList` is a mutable bind_vector, completed to a `collections.abc.MutableSequence`.
  // Its element access copies the point (nanobind's default policy for a bound vector), so no
  // Python object ever refers into the vector's storage and `append`, `pop`, `del` and slice
  // assignment cannot leave a dangling wrapper behind. The price is that `points[0].latitude = x`
  // modifies a temporary; the documented way to change a point is to assign `points[0] = point`.
  // Do not switch the policy to reference_internal: it would make that write-through work but
  // reintroduce use-after-free through the mutators.
  //
  // `SegmentList` and `TrackList` cannot copy on access, as a Track copies every point in it, so
  // they return references and are read-only; see BindReadOnlySequence.
  //
  // Iterating a LatLongList creates one wrapper per point as the old list conversion did, but
  // lazily; the copy makes each wrapper about 1.3x dearer than a reference wrapper was. Building
  // the whole list in one C++ pass instead was tried and did not measure as faster.
  auto latlong_list = nb::bind_vector<std::vector<LatLong>>(
      m, "LatLongList", nb::sig("class LatLongList(collections.abc.MutableSequence[LatLong])"));
  // bind_vector provides the rest of the MutableSequence protocol. Its generated methods take
  // their arguments positionally, so a checker reading the stub notes that they do not accept the
  // keyword forms `collections.abc` declares; the methods defined here follow the ABC's names.
  latlong_list
      .def(
          "index",
          [](const std::vector<LatLong>& v, const LatLong& value, Py_ssize_t start,
             std::optional<Py_ssize_t> stop) {
            const auto [first, last] = ClampRange(start, stop, v.size());
            for (size_t i = first; i < last; ++i)
            {
              if (v[i] == value)
              {
                return static_cast<Py_ssize_t>(i);
              }
            }
            throw nb::value_error("value is not in the sequence");
          },
          "value"_a, "start"_a = 0, "stop"_a.none() = nb::none())
      .def("reverse", [](std::vector<LatLong>& v) { std::reverse(v.begin(), v.end()); })
      // `reversed()` iterates a reversed slice, a LatLongList copy taken up front. An iterator
      // holding C++ iterators would be invalidated by `append` or `clear` while it is alive.
      .def("__reversed__",
           [](nb::handle_t<std::vector<LatLong>> self) {
             return nb::typed<nb::iterator, LatLong>(
                 nb::iter(self[nb::slice(nb::none(), nb::none(), nb::int_(-1))]));
           })
      // `index` above has a fallback for a value that is not a point, so that it answers as
      // `list` does instead of raising TypeError. `count` and `remove` need the same, but
      // bind_vector's versions take the value positionally only, and a keyword call would skip
      // them and land on the fallback with a wrong answer. Both are therefore replaced with
      // versions that accept `value=` before the fallback is added.
      .def(
          "index",
          [](const std::vector<LatLong>&, nb::handle, Py_ssize_t, std::optional<Py_ssize_t>)
              -> Py_ssize_t { throw nb::value_error("value is not in the sequence"); },
          "value"_a, "start"_a = 0, "stop"_a.none() = nb::none());
  nb::delattr(latlong_list, "count");
  nb::delattr(latlong_list, "remove");
  latlong_list
      .def(
          "count",
          [](const std::vector<LatLong>& v, const LatLong& value) {
            return std::count(v.begin(), v.end(), value);
          },
          "value"_a)
      .def(
          "count", [](const std::vector<LatLong>&, nb::handle) { return 0; }, "value"_a)
      .def(
          "remove",
          [](std::vector<LatLong>& v, const LatLong& value) {
            const auto it = std::find(v.begin(), v.end(), value);
            if (it == v.end())
            {
              throw nb::value_error("value is not in the sequence");
            }
            v.erase(it);
          },
          "value"_a)
      .def(
          "remove",
          [](std::vector<LatLong>&, nb::handle) {
            throw nb::value_error("value is not in the sequence");
          },
          "value"_a)
      .def(
          "__iadd__",
          [](nb::handle_t<std::vector<LatLong>> self, nb::handle other) {
            // bind_vector's `extend` handles extending a vector with itself and converts any
            // iterable of points.
            self.attr("extend")(other);
            return nb::borrow(self);
          },
          nb::sig("def __iadd__(self, values: LatLongList | collections.abc.Iterable[LatLong]) "
                  "-> LatLongList"));
  // Value equality without a matching hash: unhashable, as `list` is.
  latlong_list.attr("__hash__") = nb::none();
  RegisterAbc(latlong_list, "MutableSequence");
  latlong_list.doc() =
      "The points of a :class:`Segment`, a mutable sequence of :class:`LatLong` backed by the "
      "segment's own storage. Editing ``segment.points`` edits the segment. "
      "``LatLongList(iterable)`` builds one from any iterable of points, and a plain sequence of "
      "points is accepted wherever a ``LatLongList`` is expected.\n\n"
      "Indexing and iteration return *copies* of the points, and a slice is a new, independent "
      "``LatLongList``. Assigning to an attribute of an element therefore does not change the "
      "segment. To change a point, assign it back::\n\n"
      "    point = segment.points[0]\n"
      "    point.latitude = 60.0\n"
      "    segment.points[0] = point";

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
                                                std::numeric_limits<double>::lowest());
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
                                                 std::numeric_limits<double>::lowest());
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
      // The setter also accepts any sequence of points through LatLongList's implicit
      // conversion, which the generated signature would not show.
      .def_rw("points", &Segment::points,
              nb::for_setter(nb::sig("def points(self, arg: LatLongList | "
                                     "collections.abc.Sequence[LatLong], /) -> None")),
              "The track points as a :class:`LatLongList`.")
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

  BindReadOnlySequence<std::vector<Segment>>(m, "SegmentList",
                                             "class SegmentList(collections.abc.Sequence[Segment])")
      .doc() =
      "The segments of a :class:`Track`, a read-only sequence of the track's own "
      ":class:`Segment` objects. Changes made through an element are visible in the "
      "track, and each element keeps its document alive.";

  nb::class_<Track>(m, "Track")
      .def(nb::init<>()) // Default constructor
      .def_rw("name", &Track::name)
      .def_rw("comment", &Track::comment)
      .def_rw("description", &Track::description)
      .def_rw("number", &Track::number)
      .def_rw("type", &Track::type)
      .def_ro("segments", &Track::segments, "The segments as a :class:`SegmentList`.")
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

  BindReadOnlySequence<std::vector<Track>>(m, "TrackList",
                                           "class TrackList(collections.abc.Sequence[Track])")
      .doc() =
      "The tracks of a :class:`Gpx`, a read-only sequence of the document's own "
      ":class:`Track` objects. Changes made through an element are visible in the "
      "document, and each element keeps the document alive.";

  nb::class_<Gpx>(m, "Gpx")
      .def(nb::init<>()) // Default constructor
      .def_ro("tracks", &Gpx::tracks, "The tracks as a :class:`TrackList`.")
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

  // Parsing is pure C++ and touches no Python objects, so the GIL is released for the duration
  // and several threads can load files at once. nanobind enters the guard after the arguments
  // have been converted and leaves it before the result is converted or an exception translated.
  // nanobind's std::filesystem::path caster renders its parameter as a bare `os.PathLike`, which
  // pyright strict reads as `PathLike[Unknown]`. The caster goes through `os.fspath`, so it takes
  // the same path types as `open()`; the signature is overridden to say so. (#60)
  m.def("load", &LoadGpx, "path"_a, nb::call_guard<nb::gil_scoped_release>(),
        nb::sig("def load(path: str | bytes | os.PathLike[str] | os.PathLike[bytes]) -> Gpx"),
        "Load and parse a GPX file.\n\n"
        "A file that cannot be read raises ``FileNotFoundError`` or ``OSError``, as any file API "
        "would; malformed content raises :class:`ParseError`.\n\n"
        "Releases the GIL while parsing, so files can be loaded from several threads at once.");
  m.def("parse", &ParseGpx, "data"_a, nb::call_guard<nb::gil_scoped_release>(),
        "Parse GPX data from a string.\n\n"
        "Releases the GIL while parsing, so data can be parsed from several threads at once.");

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

  // `encode` takes the bound LatLongList by reference, so `encode(segment.points)` copies
  // nothing. Any other sequence of points goes through LatLongList's implicit conversion, which
  // the generated signature would not show, hence the explicit ones.
  polyline_mod.def(
      "encode",
      [](const std::vector<LatLong>& points, polyline::Precision precision) {
        return polyline::encode(points, precision);
      },
      "locations"_a, "precision"_a = polyline::Precision::Five,
      nb::sig("def encode(locations: fastgpx.LatLongList | "
              "collections.abc.Sequence[fastgpx.LatLong], "
              "precision: fastgpx.polyline.Precision = fastgpx.polyline.Precision.Five) -> str"));

  polyline_mod.def(
      "encode",
      [](const std::vector<LatLong>& points, int precision) {
        return polyline::encode(points, IntToPrecision(precision));
      },
      "locations"_a, "precision"_a = 5,
      nb::sig("def encode(locations: fastgpx.LatLongList | "
              "collections.abc.Sequence[fastgpx.LatLong], precision: int = 5) -> str"));

  // `decode` returns a plain list: the result is a fresh value, not a view into a document.
  polyline_mod.def(
      "decode",
      [](const std::string_view encoded, polyline::Precision precision) {
        return LatLongsToList(polyline::decode(encoded, precision));
      },
      "encoded"_a, "precision"_a = polyline::Precision::Five);

  polyline_mod.def(
      "decode",
      [](const std::string_view encoded, int precision) {
        return LatLongsToList(polyline::decode(encoded, IntToPrecision(precision)));
      },
      "encoded"_a, "precision"_a = 5);
}
