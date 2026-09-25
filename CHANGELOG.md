# Changelog

Notable changes to fastgpx, newest release first.

## 0.8.0

fastgpx 0.8.0 is faster, gives each point its timestamp, and rejects malformed GPX that 0.7.0 accepted silently. Several changes can break existing code, so read the first section before upgrading.

### Breaking changes

**Installation**

- fastgpx now depends on `nanobind-backend`, which pip and uv install automatically.
  - The extension is built with nanobind 3 in split mode, so the nanobind runtime lives in that package instead of inside fastgpx.
  - An environment holds only one `nanobind-backend`, shared by every split-mode extension. fastgpx asks for `>=1.0` with no upper bound, so that it does not clash with other packages that use it.
- There is no 32-bit Windows (`win32`) wheel any more; 0.7.0 had one.
  - fastgpx cannot be installed on 32-bit Windows at all, because `nanobind-backend` has no build for it.
  - Wheels remain for Windows x64 and ARM64 and for Linux (manylinux) x86_64 and aarch64, all for CPython 3.12 and later.

**Points, segments and tracks are no longer lists** (#18)

- `Segment.points` is a `LatLongList`, `Track.segments` a `SegmentList` and `Gpx.tracks` a `TrackList`.
- Iteration, `len()`, indexing, slicing, `in`, `index()`, `count()`, `reversed()` and `list(...)` work as before.
- `isinstance(x, list)` is now `False`. Check against `collections.abc.Sequence` instead, or call `list(...)` to get a real list.
- `gpx.tracks == [...]` and `track.segments == [...]` are now always `False`. `segment.points == [...]` still compares the points.
- `gpx.tracks = ...` and `track.segments = ...` now raise `AttributeError`. `segment.points = [...]` still works.
- `segment.points.append(...)` and other edits now change the segment. In 0.7.0 they changed a throwaway copy.
- `segment.points[0]` is still a copy. To change a point, change the copy and assign it back to `segment.points[0]`.

**Errors**

- Errors are now `fastgpx.Error`, a subclass of `ValueError`, instead of `RuntimeError`. Replace `except RuntimeError` with `except fastgpx.Error`.
- Malformed GPX, polyline or timestamp data raises `fastgpx.ParseError`, a subclass of `fastgpx.Error`.
- `load()` on a missing or unreadable file raises `FileNotFoundError` or `OSError` instead of `RuntimeError`.

**Stricter parsing.** Each of these now raises `fastgpx.ParseError` where 0.7.0 returned wrong or partial data:

- A NUL byte in the document, in `parse()` (#50) or `load()` (#67). 0.7.0 silently dropped everything after it.
- XML without a `<gpx>` root element, such as a KML file or a saved HTML error page (#69). 0.7.0 returned an empty `Gpx`. An empty `<gpx/>` is still accepted.
- A `<trkpt>` whose `lat` or `lon` is missing, not a number, or outside ±90/±180 (#51). 0.7.0 read a missing or unreadable value as 0.0 and kept out-of-range values.
- A `<time>` the platform's clock cannot hold (#48). On Linux that is before about 1677 or after about 2262. Windows takes every year from 0001 to 9999. Year 0000 is rejected everywhere. Like any bad `<time>`, it raises from `time_bounds()` or `LatLong.time`, not from `load()` or `parse()`.
- `polyline.decode` on a malformed string. 0.7.0 returned wrong points.
- `polyline.encode` now raises `fastgpx.Error` for a coordinate it cannot represent: NaN, infinity, or outside ±90/±180 (#49). 0.7.0 returned a wrong string.
- An `<ele>` with text after the number, such as `12.5abc`, now reads as 0.0 instead of 12.5. A missing or non-numeric `<ele>` is still 0.0.

**Other behaviour changes**

- `LatLong`, `TimeBounds` and `Bounds` can no longer be hashed, so `hash()`, sets and dict keys of them raise `TypeError`. They compare by value and can be changed, so a hash could not stay correct. Use a tuple of the fields you need as a key, such as `(p.latitude, p.longitude)`.
- `TimeBounds(...)`, its `start_time` and `end_time` setters, and `add()` accept only a `datetime.datetime` or `None`. A `datetime.date` or `datetime.time` now raises `TypeError`; 0.7.0 turned it into midnight or a time on 1970-01-01.
- `LatLong ==` compares timestamps by the instant they name, to the microsecond (#16). 0.7.0 compared the text, so `...T10:00:00Z` and `...T10:00:00.000Z` were unequal. Points that were equal before are still equal.
- `polyline.decode`'s first argument is called `encoded` in both overloads. Code that passed `locations=` with an `int` precision must use `encoded=`.

### New

- `LatLong.time`: the point's `<time>` as a UTC `datetime.datetime`, or `None` (#12).
  - It can be set, and it is an optional fourth argument to `LatLong(...)`.
  - A naive `datetime` is taken as UTC.
  - `repr()` of a point now shows its time.
- `Segment.lonlat()` returns the points as a list of `(longitude, latitude)` tuples, built without a `LatLong` per point (#73).
  - Longitude comes first, as Shapely, GEOS and GeoJSON expect.
- `load()` and `parse()` release the GIL, so several threads can parse at once. With 4 threads, loading 24 files took 71 ms instead of 277 ms.
- `<time>` accepts any number of fractional-second digits (#15). 0.7.0 accepted none or exactly three, so `time_bounds()` failed on files with, say, the seven digits .NET writes.

### Faster

Upload-path figures are for x86_64 Linux: 106 real GPX files (1.35 million points), ns per point, median of 5 runs in one session. The "before" build is the development version just before the final round of speed work, not 0.7.0.

- Sleipnir's upload path (parse, time bounds, coordinates, segment statistics): 433.5 → 304.0 ns per point, 30% less.
- The same path using `Segment.lonlat()`: 221.5 ns per point, 49% less.
- `parse()`: 183.2 → 135.5 ns per point. The wheels are now optimised Release builds, and the Linux wheels also use link-time optimisation.
- Track time bounds: 76.0 → 10.3 ns per point. Only the earliest and latest timestamps are parsed now (#19).
- Copying points with `list(segment.points)`: 47.6 → 38.1 ns per point. A copy no longer allocates memory.
- Coordinates via `Segment.lonlat()` instead of tuples built from `list(segment.points)`: 120.6 → 37.2 ns per point.
- `polyline.encode` is about twice as fast: 2.86 → 1.49 ms for one file on Windows. `encode(segment.points)` also no longer copies the points first.
- `len(segment.points)` and `segment.points[i]` no longer build a list of every point: 1.29 ms → 0.2 µs on a 9,196-point segment.

### Fixes

- Timestamps before 1970 or after 3000 read correctly on Windows. 0.7.0 silently returned 1969-12-31 23:59:59 UTC (#19).
- Far-off timestamps on Linux no longer overflow into a wrong time; they raise `ParseError` (#48, found by fuzzing).
- A track or segment taken from a temporary document, as in `fastgpx.load(path).tracks[0]`, no longer refers to freed memory (#18).
- `polyline.decode` no longer reads past the end of a truncated string.
- `polyline.encode` no longer overflows on NaN, infinity or huge coordinates (#49, found by fuzzing).
- Coordinates read correctly when the host program has set a locale with a decimal comma, such as `de_DE`. 0.7.0 could read `61.5` as 61.
- A timezone-aware `datetime` passed to `TimeBounds` is converted to UTC. 0.7.0 ignored its offset.
- Setting `Bounds.max_latitude` or `max_longitude` on an empty `Bounds` no longer stops a later `add()` with a negative coordinate from updating the maximum.
- The type hints for `load()` pass pyright in strict mode (#60).
