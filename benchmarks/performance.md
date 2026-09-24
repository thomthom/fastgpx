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

## Earlier measurements

Recorded before this document existed, in their own notes:

- [datetime_parse.md](datetime_parse.md): the timestamp parser variants that led to `parse_gpx_time`.
- [nanobind3.md](nanobind3.md): the nanobind 3 upgrade and split mode, #59.
- [nanobind_vs_pybind11.md](nanobind_vs_pybind11.md): the move from pybind11 to nanobind.
- [gpx_parse.md](gpx_parse.md): parse times while time bounds were being added.

## How the desktop numbers were measured

The C++ rows are the Catch2 benchmarks `[!benchmark][datetime]` and `[!benchmark][timebounds]`.
The time bounds benchmark was added in `9b94af8`, so it was copied into the older commits to
measure them. The 20k-point file is `gpx/2024 TopCamp/Connected_20240518_094959_.gpx`, and the
24 files are `gpx/2024 Great Roadtrip`. The Python rows use `timeit`, taking the best of 15 repeats,
with `time_bounds()` called on freshly loaded documents, since it caches its result.

The "109 real files" rows come from [benchmark_corpus.py](benchmark_corpus.py). It runs one build
over any set of folders and compares two runs, including whether the time bounds agree. To check a
change against your own GPX files, use it the same way.
