# Changelog

Notable changes to fastgpx, newest release first.

## 0.8.0

fastgpx 0.8.0 is faster, gives each point its timestamp, and rejects malformed GPX that 0.7.0 accepted silently. Several changes can break existing code, so read the first section before upgrading.

### Breaking changes

**Installation**

- fastgpx now depends on `nanobind-backend`, which pip and uv install automatically.
  - The extension is built with nanobind 3 in split mode, so the nanobind runtime lives in that package instead of inside fastgpx.
  - An environment can contain only one `nanobind-backend`, which all split-mode extensions share. fastgpx asks for `>=1.0` with no upper bound, so that it does not clash with other packages that use it.
- 32-bit Windows is no longer supported. There is no `win32` wheel, and fastgpx can't be installed there from source either, because `nanobind-backend` has no 32-bit Windows build.
  - Wheels remain for Windows x64 and ARM64 and for Linux (manylinux) x86_64 and aarch64, all for CPython 3.12 and later.

**Points, segments and tracks are no longer lists** (#18)

- `Segment.points` is a `LatLongList`, `Track.segments` a `SegmentList` and `Gpx.tracks` a `TrackList`.
- Iteration, `len()`, indexing, slicing, `in`, `index()`, `count()`, `reversed()` and `list(...)` work as before.
- `isinstance(x, list)` is now `False`. Check against `collections.abc.Sequence` instead, or call `list(...)` to get a real list.
- `gpx.tracks == [...]` and `track.segments == [...]` are now always `False`. `segment.points == [...]` still compares the points.
- `gpx.tracks = ...` and `track.segments = ...` now raise `AttributeError`. `segment.points = [...]` still works.
- `segment.points.append(...)` and other edits now modify the segment. In 0.7.0 they modified a temporary copy.
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

- `LatLong`, `TimeBounds` and `Bounds` can no longer be hashed, so `hash()`, sets and dict keys of them raise `TypeError`. In 0.7.0 they could be hashed, but equal objects got different hashes, so sets kept duplicates and dict lookups with an equal object failed. They compare by value and can be modified, so a hash could not stay correct. Use a tuple of the fields you need as a key, such as `(p.latitude, p.longitude)`.
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
- `load()` and `parse()` release the GIL, so several threads can parse at once. With 4 threads, loading 24 files took 71 ms instead of 277 ms, on an x64 build emulated on an ARM64 Surface.
- `<time>` accepts any number of fractional-second digits (#15). 0.7.0 accepted none or exactly three, so `time_bounds()` failed on files with, say, the seven digits .NET writes.

### Faster

These figures compare the 0.7.0 and 0.8.0 wheels on one machine, under Windows x64 and under x86_64 Linux in WSL2. The workload is a typical GPX import. It decodes each file from UTF-8, parses it, and gets each track's time bounds. It then copies each segment's points to a list, builds `(longitude, latitude)` tuples from them, and computes each segment's bounds, length and time bounds. It ran over 106 real-world GPX files with 1.35 million points. Times are ns per point, the median of 5 runs.

- The typical import is 2.1× faster on Linux (619 → 299 ns per point) and 1.7× faster on Windows (951 → 573).
- The same import using `Segment.lonlat()` for the coordinates is 2.9× faster than 0.7.0 on Linux (216 ns per point) and 2.1× faster on Windows (459).
- `parse()`: 303 → 131 ns per point on Linux (2.3×) and 491 → 369 on Windows (1.3×). The wheels are now Release builds, and the Linux wheels also use link-time optimisation.
- Track time bounds: 79.5 → 10.3 ns per point on Linux (7.7×) and 190 → 13.7 on Windows (14×). Only the earliest and latest timestamps are parsed now (#19).
- Copying points with `list(segment.points)` and freeing the list: 115 → 50.4 ns per point on Linux (2.3×) and 133 → 61.3 on Windows (2.2×).
- `polyline.encode` over 24 files, including loading them: 0.176 → 0.0464 s on Linux (3.8×) and 0.281 → 0.155 s on Windows (1.8×). `encode(segment.points)` also no longer copies the points first.
- `len(segment.points)` and `segment.points[i]` no longer build a list of every point. On a 9,196-point segment, `len()` went from 1.29 ms to 0.2 µs. This was measured during development, on an x64 build emulated on an ARM64 Surface.

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
