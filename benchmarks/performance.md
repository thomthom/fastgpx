# Performance history

One section per change that was made for speed, with before and after numbers. Lower is better.
Each "before" is the commit just before the change, built and run on the same machine as its
"after". Absolute numbers are only comparable within a row; compare the speedup across machines.

## Machines

| Machine | CPU | OS | Build | Notes |
|---------|-----|----|-------|-------|
| Desktop | AMD Ryzen 7 5800X, 8 cores / 16 threads, 32 GB | Windows 11 Pro 25H2 | MSVC 19.51 x64 RelWithDebInfo, CPython 3.12 | Native x64 |
| Surface | Snapdragon X Plus X1P64100 | Windows 11 | MSVC 19.44 x64 RelWithDebInfo | x64 build running under ARM64 emulation |

Desktop numbers are the median of five runs, with the before and after builds run in alternation.
Surface numbers are the median of three runs, taken from the commit messages; a dash means that
measurement was not made on that machine.

## Test files

Single-file rows use `gpx/2024 TopCamp/Connected_20240518_094959_.gpx` (1.9 MB, 20k points).
Rows marked "109 real files" use a wider collection, so that no change is tuned to one file:

| | |
|---|---|
| Files | 154 unique GPX files, 150 MB, 1.5 million track points; 109 of them have 1,000 points or more |
| Sources | This repository's `gpx/` folder, plus tracks uploaded to the Sleipnir dev server and the TET country routes |
| Written by | About 20 apps and devices; mostly BMW Motorrad Connected, also Garmin, Beeline, GPSBabel, gpxpy, Runkeeper and others |
| Timestamps | 97% `2024-05-18T06:50:01Z`; the rest with milliseconds, seven fraction digits, a UTC offset or no time zone. The TET routes carry timestamps on only some of their points |

The speedups for that collection are the total over the 109 larger files. Smaller files take
microseconds, so their ratios are mostly noise.

## Faster timestamp parsing (`dev/faster-time-bounds`)

Commit `8c87bf1`, #19. Timestamps are converted to a time point with plain date arithmetic instead
of a call into the C runtime. This also makes dates before 1970 parse on Windows.

| Measurement | Desktop before | Desktop after | Speedup | Surface before | Surface after | Speedup |
|-------------|---------------:|--------------:|--------:|---------------:|--------------:|--------:|
| Parse one `<time>` string (C++) | 88 ns | 69 ns | 1.28× | 309 ns | 287 ns | 1.08× |
| Time bounds of a 5,189-point segment (C++) | 595 µs | 485 µs | 1.23× | – | – | – |
| `gpx.time_bounds()` on a 20k-point file (Python) | 4.57 ms | 3.76 ms | 1.22× | – | – | – |
| `gpx.time_bounds()`, 109 real files (Python) | 366 ms | 314 ms | 1.17× | – | – | – |

## Time bounds without parsing every timestamp (`dev/faster-time-bounds`)

Commit `9b94af8`, #19. Finding a track's start and end time used to parse every timestamp in it.
For the usual GPX timestamp formats, the text sorts in time order, so now only the earliest and
latest are parsed. Anything unusual falls back to the old path.

| Measurement | Desktop before | Desktop after | Speedup | Surface before | Surface after | Speedup |
|-------------|---------------:|--------------:|--------:|---------------:|--------------:|--------:|
| Time bounds of a 5,189-point segment (C++) | 485 µs | 143 µs | 3.4× | 1.60 ms | 0.22 ms | 7.4× |
| `gpx.time_bounds()` on a 20k-point file (Python) | 3.76 ms | 0.71 ms | 5.3× | 6.4 ms | 0.80 ms | 8.0× |
| `gpx.time_bounds()`, 109 real files (Python) | 314 ms | 49 ms | 6.4× | – | – | – |
| `load()` + `time_bounds()`, 24 files (Python) | 243 ms | 196 ms | 1.24× | – | – | – |

Across the 109 real files, the speedup per file ranges from 3.7× to 12× wherever every point has a
timestamp. The single test file sits below the middle of that range. The TET routes gain little,
since most of their points have no timestamp to parse in the first place.

## Both changes together

From `f4953bf` (before either change) to `9b94af8`, desktop only. Loading a file does not read
timestamps, so load time itself is unchanged within noise (about 12 ms for the 20k-point file).

| Measurement | Before | After | Speedup |
|-------------|-------:|------:|--------:|
| Parse one `<time>` string (C++) | 88 ns | 65 ns | 1.35× |
| `gpx.time_bounds()` on a 20k-point file (Python) | 4.57 ms | 0.71 ms | 6.5× |
| `gpx.time_bounds()`, 109 real files (Python) | 366 ms | 49 ms | 7.5× |
| `load()` + `time_bounds()`, 20k-point file (Python) | 16.1 ms | 12.6 ms | 1.27× |
| `load()` + `time_bounds()`, 24 files (Python) | 272 ms | 196 ms | 1.39× |

Every one of the 154 files gives the same time bounds before and after, with one intended
exception. `Mojstrovka.gpx` has timestamps from 1901, which the old code rejected on Windows.

## Regression check: `LatLong.time` (`dev/latlong-time`)

Commit `1b8881a`, #12 and #16. This is a feature, not a speed change. It is listed because it
changes how points compare equal. On the desktop, comparing two points with `==` stayed at about
55–70 ns, and none of the measurements above moved outside noise.

## Release build for the wheels

The wheels were built as RelWithDebInfo, which on MSVC restricts inlining and on Linux ships the
extension with about 12 MB of debug info. They are now built as Release. Desktop only; Linux is
WSL2 on the same machine, measured in C++. Details and the other settings tried are in
[build_settings.md](build_settings.md).

| Measurement | Before | After | Speedup |
|---|---:|---:|---:|
| `load()`, 183 real files (Python, Windows) | 1.50 s | 1.45 s | 1.04× |
| `time_bounds()`, 183 real files (Python, Windows) | 39.7 ms | 31.1 ms | 1.28× |
| Reading every point's coordinates (Python, Windows) | 22.1 ms | 19.5 ms | 1.13× |
| `polyline.encode`, all segments of one file (Python, Windows) | 2.93 ms | 2.61 ms | 1.12× |
| `LoadGpx`, TET files (C++, Linux) | 138 ns/point | 137 ns/point | 1.01× |
| Time bounds of a 5,189-point segment (C++, Linux) | 70 µs | 62 µs | 1.13× |
| `polyline::decode`, 10k points (C++, Linux) | 101 µs | 60 µs | 1.67× |
| Windows wheel (published 0.7.0 against a local Release build) | 370 KB | 203 KB | |

Nothing got slower. Loading, the main cost, barely changes: most of its time is in work the compiler
cannot remove (see [load_profile.md](load_profile.md)).

## Polyline encoding without a temporary string per value

The encoder built a small temporary string for every value and appended it to the result. It now
appends the characters to the result directly. This also removes the slowdown link-time
optimization caused on MSVC, which is explained in [build_settings.md](build_settings.md). Release
builds, desktop; Linux is WSL2 on the same machine.

| Measurement | Before | After | Speedup |
|---|---:|---:|---:|
| `polyline::encode`, 10k points (C++, Windows) | 165 µs | 74 µs | 2.2× |
| `polyline::encode`, 10k points (C++, Linux) | 142 µs | 72 µs | 2.0× |
| `polyline.encode`, all segments of one file (Python, Windows) | 2.86 ms | 1.49 ms | 1.9× |
| `polyline::encode` with link-time optimization (C++, Windows) | 484 µs | 81 µs | 6.0× |

## Trimming numbers without a general search

Every latitude, longitude and elevation is trimmed of surrounding whitespace before it is converted.
The trim used `find_first_not_of` and `find_last_not_of`, which the profile put at about a tenth of
load time on Windows (see [load_profile.md](load_profile.md)). It now checks the first and last
character directly. Accepted input is unchanged. Release builds, desktop; Linux is WSL2 on the same
machine.

| Measurement | Before | After | Speedup |
|---|---:|---:|---:|
| `LoadGpx`, TET files (C++, Windows) | 474 ns/point | 418 ns/point | 1.13× |
| `load()`, 183 real files (Python, Windows) | 1.65 s | 1.42 s | 1.16× |
| `LoadGpx`, TET files (C++, Linux) | 139 ns/point | 143 ns/point | no change within noise |

On Linux the rounds varied by about 25%, and no gain could be seen. GCC may already have made the
old search cheap; that has not been checked.

## Link-time optimization for the Linux wheels

The Linux wheels are now built with link-time optimization, which lets the compiler inline across
source files, for example pugixml's lookups into fastgpx's loop. Windows wheels are not, because
there it made polyline decoding slower. Linux is WSL2 on the desktop, GCC 14, measured in C++.
Details are in [build_settings.md](build_settings.md).

| Measurement | Before | After | Speedup |
|---|---:|---:|---:|
| `LoadGpx`, TET files | 137 ns/point | 112 ns/point | 1.22× |
| Parse a 20k-point file | 3.94 ms | 3.18 ms | 1.24× |
| Time bounds of a 5,189-point segment | 61.6 µs | 55.3 µs | 1.12× |
| `parse_gpx_time`, one timestamp | 24 ns | 35 ns | 0.69× |

Single-timestamp parsing gets slower, but bulk timestamp work (time bounds) is still faster.

## Earlier measurements

Recorded before this document existed, in their own notes:

- [datetime_parse.md](datetime_parse.md): the timestamp parser variants that led to `parse_gpx_time`.
- [nanobind3.md](nanobind3.md): the nanobind 3 upgrade and split mode, #59.
- [nanobind_vs_pybind11.md](nanobind_vs_pybind11.md): the move from pybind11 to nanobind.
- [gpx_parse.md](gpx_parse.md): parse times while time bounds were being added.

For where load time goes today on Windows and Linux, see [load_profile.md](load_profile.md). For
the fastgpx side of Sleipnir's GPX upload, including the Python work around parsing, see
[ingest_profile.md](ingest_profile.md).

## How the desktop numbers were measured

The C++ rows are the Catch2 benchmarks `[!benchmark][datetime]` and `[!benchmark][timebounds]`.
The time bounds benchmark was added in `9b94af8`, so it was copied into the older commits to
measure them. The 20k-point file is `gpx/2024 TopCamp/Connected_20240518_094959_.gpx`, and the
24 files are `gpx/2024 Great Roadtrip`. The Python rows use `timeit`, taking the best of 15 repeats,
with `time_bounds()` called on freshly loaded documents, since it caches its result.

The "109 real files" rows come from [benchmark_corpus.py](benchmark_corpus.py). It runs one build
over any set of folders and compares two runs, including whether the time bounds agree. To check a
change against your own GPX files, use it the same way.
