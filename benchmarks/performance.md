# Performance

The current numbers for fastgpx's speed, in one place. Lower is better. "Where things stand" says
how fast Sleipnir's upload path is today. After it comes one section per change made for speed,
newest first, with before and after numbers. Figures that do not meet the bar for a verified
result are kept below the line, under "Historical measurements". The full tables and build hashes
for the recent changes are in [review_decisions.md](review_decisions.md).

A result counts as verified when it was measured:

- on the commit it describes, or on current code;
- on files that can be identified: the manifest-tracked `gpx/sleipnir` and `gpx/TET`, or a named
  single file or folder;
- over several runs;
- with the build recorded.

## Where things stand

Production is x86_64 Linux, confirmed with `uname -m` on the Sleipnir server. (Sleipnir's deployment
plan specifies a Hetzner CX22; the server type itself was not checked.)

The figures below are Sleipnir's upload path, timed by
[benchmark_ingest.py](benchmark_ingest.py) over `gpx/sleipnir` (106 files, 1.35 million points).
They are ns per point, the sum of each file's best of 5 rounds, median of 5 runs. The machine is
the desktop below, under WSL2 (x86_64, GCC 14.2), with wheels built locally from each commit's
own `pyproject.toml`.

### `main` against the branch, one session (2026-09-25, quiet machine)

`main` as shipped (`f4953bf`, RelWithDebInfo per its own `pyproject.toml`) against the branch at
`9016114` (Release with link-time optimization), measured in one session on 2026-09-25 with the
machine quiet (no browser, editor or games). The two builds alternated run by run. `no_copy` is
the upload path after thomthom/sleipnir#596. `lonlat` is the same path with `segment.lonlat()`,
which Sleipnir does not use yet; `main` does not have it.

| Step | `main` as shipped, `no_copy` | branch, `no_copy` | branch, `lonlat` |
|---|---:|---:|---:|
| `content.decode("utf-8")` | 5.7 | 5.0 | 4.7 |
| `fastgpx.parse(text)` | 183.2 | 135.5 | 136.1 |
| track time bounds | 76.0 | 10.3 | 10.3 |
| `list(segment.points)` | 47.6 | 38.1 | – |
| `(lon, lat)` tuples from the points | 74.8 | 70.1 | – |
| `segment.lonlat()` | – | – | 38.6 |
| segment bounds, length, time bounds | 33.1 | 31.6 | 31.4 |
| freeing the point lists | 15.2 | 13.2 | – |
| merge bounds | 0.3 | 0.3 | 0.3 |
| **total** | **433.5** | **304.0** | **221.5** |

The total is the median of the per-run totals, so the steps need not add up to it exactly. Per-run
totals: `main` 427.3–457.4, branch `no_copy` 298.5–335.1, branch `lonlat` 215.6–249.0. The branch
was faster in every one of the 5 run pairs. As it stands, the branch takes 30% off `main`'s upload
path (1.43×), and 49% (1.96×) once Sleipnir calls `segment.lonlat()`.

### Per-change measurements

How the total got from `main` to the branch, one measured step at a time. Each row compares two
builds, or for `b841e82` two variants on one build, run in alternation in one session. These
sessions were taken with the machine in normal use. The first two rows share a session, measured
for `b01d107` on the branch as it stood at `3b35bae`:

| Commit | Change | Before | After |
|---|---|---:|---:|
| `b01d107` | `main` against the branch, both Release + link-time optimization | 423.6 | 382.6 |
| `b01d107` | the branch without link-time optimization | 382.6 | 367.6 |
| `7a03e1d` | hidden visibility, `NOMINSIZE` and link-time optimization | 359.4 | 322.4 |
| `24b3d2f` | timestamp text stored inside the point | 338.0 | 307.5 |
| `b841e82` | `no_copy` against `lonlat`, same build | 297.7 | 214.3 |

The "before" of each session is not the "after" of the one above it: 367.6 against 359.4, 322.4
against 338.0, 307.5 against 297.7. Sessions differ by up to about 16 ns per point, so compare
within a row.

Not covered by these figures:

- Production's wheel is built by cibuildwheel in the `manylinux_2_28` image, not with Ubuntu's
  GCC. A reviewer's `manylinux_2_28` build of `7a03e1d` showed the same gain as the table (parse
  175.1 → 144.8, total 392.1 → 347.9). The later changes were not measured in that image.
- Production gains none of this until the branch is in a fastgpx release that Sleipnir installs.
  The `lonlat` column also needs Sleipnir to call it (thomthom/sleipnir#597). See the open points
  in [review_decisions.md](review_decisions.md).

## Machines

| Machine | CPU | OS | Build | Notes |
|---------|-----|----|-------|-------|
| Desktop | AMD Ryzen 7 5800X, 8 cores / 16 threads, 32 GB | Windows 11 Pro 25H2 | MSVC 19.51 x64, CPython 3.12 | Native x64; Linux rows are WSL2 on it, GCC 14.2 |
| Surface | Snapdragon X Plus X1P64100 | Windows 11 | MSVC 19.44 x64 RelWithDebInfo | x64 build running under ARM64 emulation |

The build type is that of the wheels at the time. Sections up to and including the regression check
of `LatLong.time` used RelWithDebInfo; from "Release build for the wheels" on, Release (plus
link-time optimization where a section says so).

Unless a section says otherwise, each "before" is the commit just before the change, built and run
on the same machine as its "after".

Desktop numbers are the median of five runs, with the before and after builds run in alternation.
Surface numbers are the median of three runs, taken from the commit messages; a dash means that
measurement was not made on that machine. Absolute numbers are only comparable within a row;
compare the speedup across machines.

## Test files

Single-file rows use `gpx/2024 TopCamp/Connected_20240518_094959_.gpx` (1.9 MB, 20k points).
Rows marked "24 files" use `gpx/2024 Great Roadtrip`. The newer rows use the folders listed in
[corpus_manifest.json](corpus_manifest.json): `gpx/sleipnir` (106 unique upload files, 1.35
million points) and `gpx/TET` (34 files, 1.66 million points).
`uv run benchmarks/corpus_manifest.py verify` checks a local copy against it.

## Verified results, newest first

### Bulk coordinate accessor: `Segment.lonlat()` (`dev/latlong-time-review`)

Commit `b841e82`, #73. `Segment.lonlat()` builds the `(longitude, latitude)` tuples that
Sleipnir hands to GEOS in C++, without a Python object per point. It is a new method, so the
upload path gains only once Sleipnir calls it. `gpx/sleipnir`, ns per point, median of 5 runs.
`no_copy` and `lonlat` alternate within each run, on the same build:

| Measurement | `no_copy` | `lonlat` | Speedup |
|---|---:|---:|---:|
| Coordinates: point list, tuples and freeing (Linux) | 120.6 | 37.2 | 3.2× |
| Upload path total, Linux | 297.7 | 214.3 | 1.39× |
| Upload path total, Windows | 777.9 | 625.0 | 1.24× |

Linux is the wheel configuration of `pyproject.toml`; Windows is the editable Release build. The
Windows session was probably busy, so compare its two columns only with each other. The per-step
table is in [review_decisions.md](review_decisions.md).

### Inline timestamp text (`dev/latlong-time-review`)

Commit `24b3d2f`. Since `9b94af8`, points keep their `<time>` text, and a timestamp is too long for
`std::string`'s built-in buffer, so parsing, copying and freeing a point each allocated. The text
is now stored inside the point (up to 38 characters). Equality now compares at microsecond
resolution and `LatLong` gained a matching hash (since dropped: `LatLong` is now unhashable).
Linux as in the next section. Windows is the Release wheel of each commit, median of 5 alternated
runs, re-measured on a quiet machine on 2026-09-25 (the first session was noisy; both are in the
decision log):

| Measurement | Before | After | Speedup |
|---|---:|---:|---:|
| `fastgpx.parse(text)`, Linux | 142.7 | 135.7 | 1.05× |
| `list(segment.points)`, Linux | 55.2 | 40.1 | 1.38× |
| Freeing the point lists, Linux | 20.9 | 13.3 | 1.57× |
| Upload path total, Linux | 338.0 | 307.5 | 1.10× |
| Upload path total, Windows | 630.0 | 575.4 | 1.09× |
| Copy and free 19,962 points (C++, Linux) | 525 µs | 81 µs | 6.5× |
| Copy and free 19,962 points (C++, Windows) | 1,489 µs | 374 µs | 4.0× |

### Link-time optimization that reaches the parser (`dev/latlong-time-review`)

Commits `b01d107` and `7a03e1d`. In the extension, link-time optimization did not reach the parser.
GCC's inlining report points to two reasons: pugixml's functions could be replaced at load time,
so GCC would not inline them, and nanobind's `-Os` copies of shared helpers could not be inlined
into `-O3` code. Measured on Sleipnir's upload path from Python, it made parsing slower (186
against 169 ns per point), so `b01d107` made the wheels Release everywhere. `7a03e1d` then built
pugixml and the core library with hidden visibility and the Linux extension with nanobind's
`NOMINSIZE`, and turned link-time optimization back on for Linux. Windows wheels stay Release
without it.

Linux, `gpx/sleipnir` (106 files, 1.35 million points), the upload path after
thomthom/sleipnir#596 (`no_copy`), ns per point, median of 5 alternated runs:

| Measurement | Release | Release, hidden, `NOMINSIZE`, LTO | Speedup |
|---|---:|---:|---:|
| `fastgpx.parse(text)` | 167.4 | 136.6 | 1.23× |
| `list(segment.points)` | 58.2 | 50.4 | 1.15× |
| Upload path total | 359.4 | 322.4 | 1.11× |
| Upload path total, `gpx/TET` | 320.5 | 301.6 | 1.06× |
| `parse_gpx_time` (C++) | 23.6 ns | 35.0 ns | 0.67× |

The `parse_gpx_time` cost does not reach the upload path, which never reads a point's time; time
bounds got faster (11.7 → 9.8 ns per point). A reviewer's build in the `manylinux_2_28` image
showed the same gain (parse 175.1 → 144.8, total 392.1 → 347.9). Linux ARM is unmeasured, and
does not matter for production, which is x86_64.

### Link-time optimization for the Linux wheels (reverted)

`58189f2` built the Linux wheels with link-time optimization, which lets the compiler inline across
source files, for example pugixml's lookups into fastgpx's loop. Windows wheels were not, because
there it made polyline decoding slower. Linux is WSL2 on the desktop, GCC 14, measured in C++.
Details are in [build_settings.md](build_settings.md).

| Measurement | Before | After | Speedup |
|---|---:|---:|---:|
| `LoadGpx`, TET files | 137 ns/point | 112 ns/point | 1.22× |
| Parse a 20k-point file | 3.94 ms | 3.18 ms | 1.24× |
| Time bounds of a 5,189-point segment | 61.6 µs | 55.3 µs | 1.12× |
| `parse_gpx_time`, one timestamp | 24 ns | 35 ns | 0.69× |

Single-timestamp parsing gets slower, but bulk timestamp work (time bounds) is still faster. Other
sessions measured the same `parse_gpx_time` pair as 24 → 36 ns and 25 → 36 ns
([build_settings.md](build_settings.md)), and 23.6 → 35.0 ns with the build of the previous
section.

This build was later found to make parsing from Python slower, not faster, and was reverted. See
the previous section.

### Trimming numbers without a general search

Commit `441ba44`. Every latitude, longitude and elevation is trimmed of surrounding whitespace
before it is converted. The trim used `find_first_not_of` and `find_last_not_of`, which the profile
put at about a tenth of load time on Windows (see [load_profile.md](load_profile.md)). It now
checks the first and last character directly. Accepted input is unchanged. Release builds,
desktop; Linux is WSL2 on the same machine.

| Measurement | Before | After | Speedup |
|---|---:|---:|---:|
| `LoadGpx`, TET files (C++, Windows) | 474 ns/point | 418 ns/point | 1.13× |
| `LoadGpx`, TET files (C++, Linux) | 139 ns/point | 143 ns/point | no change within noise |

On Linux the rounds varied by about 25%, and no gain could be seen. [load_profile.md](load_profile.md)
put the trim at 13% of Linux load time, but that profile was of a RelWithDebInfo (`-O2`) build,
while these rounds are Release (`-O3`). The likely reason is that GCC at `-O3` already made the old
search cheap. That is unconfirmed: the Release build was not profiled.

A `load()` row over the 183-file collection is under "Historical measurements".

### Polyline encoding without a temporary string per value

Commit `e66b8a2`. The encoder built a small temporary string for every value and appended it to
the result. It now appends the characters to the result directly. This also removes the slowdown
link-time optimization caused on MSVC, which is explained in [build_settings.md](build_settings.md).
Release builds, desktop; Linux is WSL2 on the same machine.

| Measurement | Before | After | Speedup |
|---|---:|---:|---:|
| `polyline::encode`, 10k points (C++, Windows) | 165 µs | 74 µs | 2.2× |
| `polyline::encode`, 10k points (C++, Linux) | 142 µs | 72 µs | 2.0× |
| `polyline.encode`, all segments of one file (Python, Windows) | 2.86 ms | 1.49 ms | 1.9× |
| `polyline::encode` with link-time optimization (C++, Windows) | 484 µs | 81 µs | 6.0× |

The Python row is `gpx/TET/F.gpx`, median of 5 runs, each the best of 7. That file was identified
afterwards, from the session transcript.

### Release build for the wheels

Commit `4c0b6be`. The wheels were built as RelWithDebInfo, which on MSVC restricts inlining and on
Linux ships the extension with about 12 MB of debug info. They are now built as Release. Desktop
only; Linux is WSL2 on the same machine, measured in C++. Details and the other settings tried are
in [build_settings.md](build_settings.md).

| Measurement | Before | After | Speedup |
|---|---:|---:|---:|
| Reading every point's coordinates (Python, Windows) | 22.1 ms | 19.5 ms | 1.13× |
| `polyline.encode`, all segments of one file (Python, Windows) | 2.93 ms | 2.61 ms | 1.12× |
| `LoadGpx`, TET files (C++, Linux) | 138 ns/point | 137 ns/point | 1.01× |
| Time bounds of a 5,189-point segment (C++, Linux) | 70 µs | 62 µs | 1.13× |
| `polyline::decode`, 10k points (C++, Linux) | 101 µs | 60 µs | 1.67× |
| Windows wheel (published 0.7.0 against a local Release build) | 370 KB | 203 KB | |

The two Python rows are `gpx/TET/F.gpx`, median of 5 runs, each the best of 7. That file was
identified afterwards, from the session transcript.

Nothing got slower. Loading, the main cost, barely changes: most of its time is in work the compiler
cannot remove (see [load_profile.md](load_profile.md)). The Windows rows from Python over the
183-file collection are under "Historical measurements".

### Both changes together

From `f4953bf` (before either change) to `9b94af8`, desktop only. Loading a file does not read
timestamps, so load time itself is unchanged within noise (about 12 ms for the 20k-point file).

| Measurement | Before | After | Speedup |
|-------------|-------:|------:|--------:|
| `gpx.time_bounds()` on a 20k-point file (Python) | 4.57 ms | 0.71 ms | 6.4× |
| `load()` + `time_bounds()`, 20k-point file (Python) | 16.1 ms | 12.6 ms | 1.28× |
| `load()` + `time_bounds()`, 24 files (Python) | 272 ms | 196 ms | 1.39× |

The `parse_gpx_time` row of this table, which mixes two sessions, and the rows over the 109 real
files are under "Historical measurements".

### Time bounds without parsing every timestamp (`dev/faster-time-bounds`)

Commit `9b94af8`, #19. Finding a track's start and end time used to parse every timestamp in it.
For the usual GPX timestamp formats, the text sorts in time order, so now only the earliest and
latest are parsed. Anything unusual falls back to the old path.

| Measurement | Desktop before | Desktop after | Speedup | Surface before | Surface after | Speedup |
|-------------|---------------:|--------------:|--------:|---------------:|--------------:|--------:|
| Time bounds of a 5,189-point segment (C++) | 485 µs | 143 µs | 3.4× | 1.60 ms | 0.22 ms | 7.4× |
| `gpx.time_bounds()` on a 20k-point file (Python) | 3.76 ms | 0.71 ms | 5.3× | –¹ | –¹ | – |
| `load()` + `time_bounds()`, 24 files (Python) | 243 ms | 196 ms | 1.24× | – | – | – |

¹ The Surface figure for this row is a single run from the commit message; it is under "Historical
measurements".

### Faster timestamp parsing (`dev/faster-time-bounds`)

Commit `8c87bf1`, #19. Timestamps are converted to a time point with plain date arithmetic instead
of a call into the C runtime. This also makes dates before 1970 parse on Windows.

| Measurement | Desktop before | Desktop after | Speedup | Surface before | Surface after | Speedup |
|-------------|---------------:|--------------:|--------:|---------------:|--------------:|--------:|
| Parse one `<time>` string (C++) | 88 ns | 69 ns | 1.28× | 309 ns | 287 ns | 1.08× |
| Time bounds of a 5,189-point segment (C++) | 595 µs | 485 µs | 1.23× | – | – | – |
| `gpx.time_bounds()` on a 20k-point file (Python) | 4.57 ms | 3.76 ms | 1.22× | – | – | – |

The row over the 109 real files is under "Historical measurements".

### How the desktop numbers were measured

The C++ rows are the Catch2 benchmarks `[!benchmark][datetime]` and `[!benchmark][timebounds]`.
The time bounds benchmark was added in `9b94af8`, so it was copied into the older commits to
measure them. The 20k-point file is `gpx/2024 TopCamp/Connected_20240518_094959_.gpx`, and the
24 files are `gpx/2024 Great Roadtrip`. The Python rows use `timeit`, taking the best of 15 repeats,
with `time_bounds()` called on freshly loaded documents, since it caches its result.

The upload-path rows come from [benchmark_ingest.py](benchmark_ingest.py). The multi-run numbers
behind the recent sections are recorded in full in [review_decisions.md](review_decisions.md),
with the builds by extension MD5 and the range over runs. Older commit messages keep their original
single-run figures; where they disagree with the multi-run figures, the decision log is the one to
trust.

For the notes written before this document, and for the profiles of where load and upload time
go, see [README.md](README.md).

---

## Historical measurements

Everything below falls short of the bar for a verified result. It is kept as it was written, so
old commit messages and discussions can still be traced. Each block says why it is here. Compare
these rows by their ratios only, and prefer the verified sections above where both cover the same
change.

### The 154- and 183-file collections

Historical because the files behind these collections were not recorded and cannot be rebuilt.

Rows marked "109 real files" use a wider collection, so that no change is tuned to one file:

| | |
|---|---|
| Files | 154 unique GPX files, 150 MB, 1.5 million track points; 109 of them have 1,000 points or more |
| Sources | This repository's `gpx/` folder, plus tracks uploaded to the Sleipnir dev server and the TET country routes |
| Written by | About 20 apps and devices; mostly BMW Motorrad Connected, also Garmin, Beeline, GPSBabel, gpxpy, Runkeeper and others |
| Timestamps | 97% `2024-05-18T06:50:01Z`; the rest with milliseconds, seven fraction digits, a UTC offset or no time zone. The TET routes carry timestamps on only some of their points |

The speedups for that collection are the total over the 109 larger files. Smaller files take
microseconds, so their ratios are mostly noise.

That collection was not recorded, and cannot be rebuilt today. Nor can the "183 real files" of the
Release and trim sections: most likely all 145 files of Sleipnir's upload folder, duplicates
included, plus the 34 TET routes and the 4 in Sleipnir's `gpx/tet`, but that is unconfirmed (see
[review_decisions.md](review_decisions.md)). Compare those rows by their ratios only.

The "109 real files" rows come from [benchmark_corpus.py](benchmark_corpus.py). It runs one build
over any set of folders and compares two runs, including whether the time bounds agree. To check a
change against your own GPX files, use it the same way.

### Rows over the 109 real files (`8c87bf1`, `9b94af8`)

Historical because they are over the unrecorded 154-file collection.

Faster timestamp parsing, `8c87bf1`:

| Measurement | Desktop before | Desktop after | Speedup | Surface before | Surface after | Speedup |
|-------------|---------------:|--------------:|--------:|---------------:|--------------:|--------:|
| `gpx.time_bounds()`, 109 real files (Python) | 366 ms | 314 ms | 1.17× | – | – | – |

Time bounds without parsing every timestamp, `9b94af8`:

| Measurement | Desktop before | Desktop after | Speedup | Surface before | Surface after | Speedup |
|-------------|---------------:|--------------:|--------:|---------------:|--------------:|--------:|
| `gpx.time_bounds()`, 109 real files (Python) | 314 ms | 49 ms | 6.4× | – | – | – |

Across the 109 real files, the speedup per file ranges from 3.7× to 12× wherever every point has a
timestamp. The single test file sits below the middle of that range. The TET routes gain little,
since most of their points have no timestamp to parse in the first place.

Both changes together, `f4953bf` to `9b94af8`:

| Measurement | Before | After | Speedup |
|-------------|-------:|------:|--------:|
| `gpx.time_bounds()`, 109 real files (Python) | 366 ms | 49 ms | 7.5× |

Every one of the 154 files gives the same time bounds before and after, with one intended
exception. `Mojstrovka.gpx` has timestamps from 1901, which the old code rejected on Windows.

### `parse_gpx_time` across two sessions (`f4953bf` to `9b94af8`)

Historical because the two figures come from different measurement sessions.

| Measurement | Before | After | Speedup |
|-------------|-------:|------:|--------:|
| Parse one `<time>` string (C++) | 88 ns | 65 ns | 1.35×¹ |

¹ `9b94af8` does not touch timestamp parsing, so it cannot account for 69 → 65 ns; that difference
lies between measurement sessions. The gain from these changes is the 1.28× measured for
`8c87bf1`.

### Rows over the 183 real files (`4c0b6be`, `441ba44`)

Historical because they are over the unrecorded 183-file collection.

Release build for the wheels, `4c0b6be`:

| Measurement | Before | After | Speedup |
|---|---:|---:|---:|
| `load()`, 183 real files (Python, Windows) | 1.50 s | 1.45 s | 1.04× |
| `time_bounds()`, 183 real files (Python, Windows) | 39.7 ms | 31.1 ms | 1.28× |

Trimming numbers without a general search, `441ba44`:

| Measurement | Before | After | Speedup |
|---|---:|---:|---:|
| `load()`, 183 real files (Python, Windows) | 1.65 s | 1.42 s | 1.16× |

### Single-run Surface figure (`9b94af8`)

Historical because it is a single measurement, from the commit message.

Time bounds without parsing every timestamp, `9b94af8`, Surface:

| Measurement | Desktop before | Desktop after | Speedup | Surface before | Surface after | Speedup |
|-------------|---------------:|--------------:|--------:|---------------:|--------------:|--------:|
| `gpx.time_bounds()` on a 20k-point file (Python) | 3.76 ms | 0.71 ms | 5.3× | 6.4 ms | 0.80 ms | 8.0× |

### Regression check: `LatLong.time` (`dev/latlong-time`)

Historical because the run count behind the desktop figures was not recorded, the slower case is
a single measurement, and the equality measured here was replaced in `24b3d2f`.

Commit `1b8881a`, #12 and #16. This is a feature, not a speed change. It is listed because it
changes how points compare equal. On the desktop, comparing two points with `==` stayed at about
55–70 ns when the coordinates differ or both times are in the same form, and none of the
measurements of `8c87bf1` and `9b94af8` moved outside noise. One case is slower: a point whose time
is still text against one whose time has been parsed. The `1b8881a` commit message, measured
separately through the bindings, gives 475 ns for it against 109–122 ns for the other cases, of
which about 110 ns is the `==` call itself. Equality changed again later, to microsecond
resolution; see "Inline timestamp text" above.
