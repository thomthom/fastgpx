// UTC conversion between `std::chrono::system_clock::time_point` and `datetime.datetime`.
//
// The stock caster in <nanobind/stl/chrono.h> goes through the process's local time zone:
// Python -> C++ uses `mktime` and C++ -> Python uses `localtime`, producing a naive datetime.
// fastgpx time points are UTC (GPX timestamps are UTC by specification), so this header replaces
// that caster for `system_clock::time_point` with one that
//
// - converts C++ -> Python as UTC and returns a timezone-aware datetime with
//   `tzinfo=datetime.timezone.utc`, and
// - converts Python -> C++ honouring `tzinfo`: aware datetimes are shifted by their `utcoffset()`
//   and naive datetimes are interpreted as UTC. `datetime.date` (midnight) and `datetime.time`
//   (on 1970-01-01) are accepted like the stock caster does.
//
// The conversion is done with the C++20 calendar types instead of `time_t`, which sidesteps the
// CRT differences (`_mkgmtime` rejects dates before 1970, glibc's `timegm` does not) and lets the
// range check happen before any arithmetic that could overflow `system_clock::duration`.
//
// An explicit specialization takes precedence over the partial specialization in the nanobind
// header, so the stock `std::chrono::duration` caster (used for `utcoffset()`) and the generic
// time point caster remain available.
#pragma once

#include <chrono>
#include <cstdint>

#include <nanobind/nanobind.h>
#include <nanobind/stl/chrono.h>

namespace nanobind::detail {

// `datetime.datetime` and `datetime.timezone.utc`, resolved once. The references are leaked on
// purpose: the objects live as long as the interpreter, and a static `nb::object` would try to
// decrement them after interpreter finalization. (nanobind 2.x cached its datetime types the same
// way.)
struct utc_datetime_types
{
  PyObject* datetime_type;
  PyObject* utc;

  // Throws `python_error` if the `datetime` module cannot be imported.
  static const utc_datetime_types& get()
  {
    static const utc_datetime_types types = [] {
      module_ datetime_mod = module_::import_("datetime");
      object datetime_type = datetime_mod.attr("datetime");
      object utc = datetime_mod.attr("timezone").attr("utc");
      return utc_datetime_types{datetime_type.release().ptr(), utc.release().ptr()};
    }();
    return types;
  }
};

template<>
class type_caster<std::chrono::system_clock::time_point>
{
public:
  using type = std::chrono::system_clock::time_point;
  using duration = type::duration;

  bool from_python(handle src, uint32_t /*flags*/, cleanup_list* cleanup) noexcept
  {
    using namespace std::chrono;

    if (!src)
    {
      return false;
    }

    int yy, mon, dd, hh, min, ss, uu;
    const int rv = unpack_datetime(src.ptr(), &yy, &mon, &dd, &hh, &min, &ss, &uu, cleanup);
    if (rv <= 0)
    {
      if (rv < 0)
      {
        PyErr_Clear();
      }
      return false;
    }

    // `unpack_datetime` reads the wall clock fields and ignores `tzinfo`. Only aware datetimes
    // have a `tzinfo`; checking it first skips the `utcoffset()` call (a Python method call) for
    // every naive datetime. `datetime.date` has no `tzinfo` attribute at all.
    microseconds offset{0};
    try
    {
      object tzinfo = getattr(src, "tzinfo", none());
      if (!tzinfo.is_none())
      {
        object utcoffset = src.attr("utcoffset")();
        // A subclass may override `utcoffset()` to return something other than a timedelta.
        // `cast` would throw `cast_error` (a `std::bad_cast`, not a `python_error`) which must
        // not escape this `noexcept` function, so use `try_cast` and reject the value instead.
        if (!utcoffset.is_none() && !try_cast<microseconds>(utcoffset, offset))
        {
          return false;
        }
      }
    }
    catch (python_error& e)
    {
      e.discard_as_unraisable(src.ptr());
      return false;
    }
    catch (...)
    {
      // Nothing may escape a `noexcept` function; treat any other failure as "not convertible".
      return false;
    }

    // The fields come from a valid Python datetime (year 1..9999), so `sys_days` and the
    // microsecond total (about +-3e17) cannot overflow. `sys_days` is only days precision, so
    // widen to `seconds` before adding the time of day: `minutes` may be a 32-bit type and would
    // overflow for late years.
    const sys_days date = year{yy} / month{static_cast<unsigned>(mon)} /
                          day{static_cast<unsigned>(dd)};
    const microseconds since_epoch = duration_cast<seconds>(date.time_since_epoch()) +
                                     hours{hh} + minutes{min} + seconds{ss} +
                                     microseconds{uu} - offset;

    // `system_clock::duration` is nanoseconds on libstdc++ (about 1677..2262), so check before
    // converting; the conversion is a multiplication that would overflow silently.
    constexpr auto max_since_epoch = floor<microseconds>(duration::max());
    constexpr auto min_since_epoch = ceil<microseconds>(duration::min());
    if (since_epoch > max_since_epoch || since_epoch < min_since_epoch)
    {
      return false;
    }

    value = type{duration_cast<duration>(since_epoch)};
    return true;
  }

  static handle from_cpp(const type& src, rv_policy, cleanup_list*) noexcept
  {
    using namespace std::chrono;

    // Split into whole seconds and a non-negative sub-second part, then whole days and time of
    // day. `floor` keeps the sub-second and time-of-day parts non-negative before the epoch too.
    const auto whole_seconds = floor<seconds>(src);
    const auto us = duration_cast<microseconds>(src - whole_seconds);
    const sys_days date = floor<days>(whole_seconds);
    const year_month_day ymd{date};
    const hh_mm_ss time_of_day{whole_seconds - date};

    const utc_datetime_types* types;
    try
    {
      types = &utc_datetime_types::get();
    }
    catch (python_error& e)
    {
      e.restore();
      return handle();
    }

    // `datetime.datetime(year, month, day, hour, minute, second, microsecond, tzinfo)`. The
    // constructor raises ValueError for years outside 1..9999. Returns a new reference, or
    // nullptr with the Python error indicator set.
    return PyObject_CallFunction(types->datetime_type, "iiiiiiiO", //
                                 static_cast<int>(ymd.year()),
                                 static_cast<int>(static_cast<unsigned>(ymd.month())),
                                 static_cast<int>(static_cast<unsigned>(ymd.day())),
                                 static_cast<int>(time_of_day.hours().count()),
                                 static_cast<int>(time_of_day.minutes().count()),
                                 static_cast<int>(time_of_day.seconds().count()),
                                 static_cast<int>(us.count()), types->utc);
  }

  NB_TYPE_CASTER(type,
                 io_name("datetime.datetime | datetime.date | datetime.time", "datetime.datetime"))
};

} // namespace nanobind::detail
