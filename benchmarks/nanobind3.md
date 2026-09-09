# nanobind 3 and split mode

Measurements made for the nanobind 2.15 to 3.0.1 upgrade (#59). The C++ library is identical in
every build; only the binding layer differs. Three builds of the same sources were compared:

| Build | nanobind | Mode                                                           |
|-------|----------|----------------------------------------------------------------|
| A     | 2.15.0   | linked, `NB_STATIC` + `STABLE_ABI` (the previous configuration) |
| B     | 3.0.1    | linked, `NB_STATIC` + `STABLE_ABI`                              |
| C     | 3.0.1    | split mode, `BACKEND_MODULE nanobind_backend` (adopted)         |

Setup: MSVC 19.44 x64 RelWithDebInfo, CPython 3.12.12, `nanobind-backend` 1.0.0. The x64 binaries
ran under emulation on an ARM64 machine (Snapdragon X X1P64100, Windows 11), so the absolute
numbers are higher than on native x64 hardware; compare columns, not against other machines. The
three builds were run interleaved (A, B, C, A, B, C, ...) on an otherwise idle machine.

## Binding microbenchmarks

`timeit` over the binding paths, microseconds per call, best of three runs of nine repeats each.
The GPX data is `gpx/2024 Great Roadtrip` (24 files); the segment used for the per-segment rows has
5809 points.

| Path                                          | A 2.15 linked | B 3.0.1 linked | C 3.0.1 split | B/A  | C/A  |
|-----------------------------------------------|--------------:|---------------:|--------------:|-----:|-----:|
| `load` + `length_2d`, all files               |        243778 |         261958 |        244735 | 1.07 | 1.00 |
| `segment.points` (vector to list)             |          83.2 |           85.8 |          62.0 | 1.03 | 0.75 |
| `gpx.tracks` (vector to list, 1 track)        |         0.087 |          0.088 |         0.088 | 1.01 | 1.01 |
| `LatLong` attribute loop, 5809 points         |          1039 |           1068 |          1036 | 1.03 | 1.00 |
| `LatLong(lat, lon, ele)`                      |         0.120 |          0.113 |         0.100 | 0.94 | 0.83 |
| `latlong.latitude`                            |         0.064 |          0.066 |         0.064 | 1.03 | 1.00 |
| `gpx.time_bounds()` (object only)             |         0.093 |          0.093 |         0.089 | 1.00 | 0.96 |
| `tb.start_time` (C++ to `datetime`)           |          1.50 |           0.32 |          0.32 | 0.21 | 0.21 |
| `TimeBounds(start, end)` (`datetime` to C++)  |          2.57 |           2.44 |          0.51 | 0.95 | 0.20 |
| `TimeBounds().add(aware +02:00)`              |          1.79 |           1.70 |          0.71 | 0.95 | 0.40 |
| `TimeBounds().add(naive)`                     |          0.88 |           0.83 |          0.16 | 0.95 | 0.18 |
| `polyline.encode`, all segments of one file   |          3560 |           3532 |          2970 | 0.99 | 0.83 |
| `polyline.decode`, one segment                |           561 |            569 |           529 | 1.01 | 0.94 |
| `geo.haversine(ll, ll)`                       |         0.187 |          0.189 |         0.184 | 1.01 | 0.98 |
| `repr(TimeBounds)`                            |          5.70 |           3.02 |          3.03 | 0.53 | 0.53 |

## Benchmark scripts

`benchmarks/benchmark_polyline.py` (10 iterations, seconds) and `benchmarks/benchmark_gpx.py`
(`fastgpx` line, average seconds per pass over the 24 files), two runs each:

| Build | `fastgpx.polyline.encode` | `fastgpx` parse + `length_2d` |
|-------|--------------------------:|------------------------------:|
| A     |               3.88 / 3.90 |                 0.282 / 0.291 |
| B     |               3.83 / 3.89 |                 0.289 / 0.293 |
| C     |               3.68 / 3.69 |                 0.292 / 0.294 |

## Takeaways

- nanobind 3.0.1 in the previous linked stable ABI mode (B) is performance-neutral. Every binding
  path is within the run-to-run noise of 2.15; the 7% on the `load` row is noise (the same parse
  path measured 0.282 to 0.293 s in the scripts for all three builds).
- The 5x faster C++ to `datetime` conversion and 2x faster `repr(TimeBounds)` in B and C come
  from the rewrite of `src/cpp/python_utc_chrono_nanobind.hpp` done in the same change, not from
  nanobind: the aware datetime is now built in one constructor call with cached `datetime` and
  `timezone.utc` objects instead of pack, module import and `replace()` per call.
- Split mode (C) is where nanobind 3 pays off for a stable ABI extension. The Python-version
  specific fast paths live in the backend package, so the extension keeps the single-wheel-per-
  platform distribution and gets 17-25% on the list conversion, object construction and
  `polyline.encode` paths, and 2.5-5x on `datetime` to C++ conversion. Parsing itself is
  unaffected: it runs entirely in C++ and never crosses the binding layer.
- The price is a runtime dependency on `nanobind-backend` (no upper bound, per the nanobind
  documentation) and matching the backend's toolchain: MSVC with `/MD`, manylinux with
  libstdc++, no musllinux wheels. The existing wheel matrix already satisfies that.

## Confirmation of the committed configuration

The build produced by the committed `CMakeLists.txt` (split mode, stable ABI floor 3.12), same
setup, best of two microbenchmark runs and one run of each script, alone on the machine:

| Path                                        | Committed build | Column C above |
|---------------------------------------------|----------------:|---------------:|
| `segment.points` (vector to list)           |            65.7 |           62.0 |
| `LatLong(lat, lon, ele)`                    |           0.099 |          0.100 |
| `tb.start_time` (C++ to `datetime`)         |            0.32 |           0.32 |
| `TimeBounds(start, end)`                    |            0.53 |           0.51 |
| `TimeBounds().add(naive)`                   |            0.17 |           0.16 |
| `polyline.encode`, all segments of one file |            3114 |           2970 |
| `polyline.decode`, one segment              |             523 |            529 |
| `benchmark_polyline.py`, seconds            |            3.78 |    3.68 / 3.69 |
| `benchmark_gpx.py`, average seconds         |           0.292 |  0.292 / 0.294 |
