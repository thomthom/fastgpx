# Review decisions: `dev/latlong-time`

A review of the `dev/latlong-time` branch (LatLong and time parsing changes, link-time
optimization for the Linux wheels, and the benchmark write-ups in this folder) turned up a list
of items. They are handled one at a time on `dev/latlong-time-review`. This file records, per
item, what was found, what was done and why, and what was left alone on purpose, so the reasoning
survives into later sessions.

## Decisions made up front

- **The 183-file figure.** `performance.md` and `build_settings.md` quote a 183-file collection
  that nobody can trace. It is recorded as most likely all 145 files of Sleipnir's upload folder,
  duplicates included, plus the 34 TET country routes in `gpx/TET` and the 4 in Sleipnir's
  `gpx/tet` (145 + 34 + 4 = 183). Unconfirmed. Those folders hold 144 unique files, which matches
  neither the 140 nor the 154 quoted elsewhere; 140 does match `gpx/sleipnir` + `gpx/TET`.
- **Multi-run numbers go into `performance.md`**, not into rewritten commit messages. Old commits
  keep their single-run numbers.
- **LatLong equality** will compare at microsecond resolution and gain a value hash (option b),
  provided that costs nothing measurable on the production path. Done in a later item.
- **Link-time optimization on Linux.** Sleipnir's upload path never reads a point's `.time`, so
  the `parse_gpx_time` slowdown under LTO does not reach production. To be confirmed by
  measurement in a later item. Linux ARM wheels remain unmeasured.
- **Won't fix:** the difference in garbage-collector handling between the timing loops of
  `benchmark_corpus.py`, and the widening of time bounds at a 1 s clock edge (the values are
  correct).

## Item: benchmarks cannot be traced to their files

**Found.** The benchmark write-ups quote collections of 140, 154 and 183 files, but none says
which folders they were. The files are not in the repository, and the script behind
`ingest_profile.md` (`ingest_bench.py`) was kept outside it and is lost. Results could not be
reproduced or compared with new ones.

**Done.**

- `gpx/sleipnir/` holds one copy of each unique file (by MD5) from Sleipnir's local upload folder:
  106 of 145. It is git-ignored like `gpx/TET/`; the sdist already excludes all of `gpx`.
- `corpus_manifest.json` lists every file in `gpx/sleipnir` and `gpx/TET` with its MD5, size,
  track-point count and timed-point count, and where it came from. `corpus_manifest.py` rewrites
  it (`generate`) or checks the local folders against it (`verify`, `--full` to recount points).
- `benchmark_corpus.py` output now records the fastgpx version, how it was installed and the MD5
  of the loaded extension, the repository commit, Python, platform and machine, and each file's
  MD5. `compare` lists files found in only one run and says what set the totals are over. It
  still reads the older output.
- `benchmark_ingest.py` replaces the lost script. It repeats the fastgpx calls of Sleipnir's upload
  view and `create_gpx_file` without Django and times each step per point, best and median over
  N rounds, with the same run information and a `compare` mode. It times two variants on separate
  parses, alternating which goes first each round: the current path, with a new `LatLong` per
  point, and the path after thomthom/sleipnir#596, which builds the coordinates from the parsed
  points. Later items should judge the branch against both, since production is moving to the
  second. Freeing each segment's point lists is timed as its own step; otherwise it lands in the
  next segment's steps, or for the last segment is never timed. It is not small: about 47 ns per
  point with the copy and 29 without, on a Windows sample.
- `profile_load` now exits non-zero with a message for a missing folder or a file, a pass count
  that is not a positive integer, a file that fails to load, and a folder with no track points.

**Why.** Later items will quote numbers. They need to name their files by something checkable,
and they need to measure the path production runs, not only `load()`.

**Not done.**

- The old figures in `performance.md`, `build_settings.md` and `ingest_profile.md` were not
  re-measured or relabelled; that belongs to the item that re-runs them.
- `segment_to_polyline` is not in `benchmark_ingest.py`: in Sleipnir only its tests call it, so
  polyline encoding is not on the upload path.
- The manifest does not cover Sleipnir's `gpx/tet` (4 files) or the other folders under `gpx/`.
- `profile_load` was built and checked with MSVC only; it is not a parser change, so the
  sanitized Clang build was not run for it.

## Item: is the branch a win for Sleipnir's upload path, and does link-time optimization help it

**Found.** On `gpx/sleipnir`, the branch is faster than `main` on both platforms and both
variants of the upload path, built the same way. With the post-thomthom/sleipnir#596 path
(`no_copy`), which production is moving to, the fastgpx time per point falls by 10% on Linux and
20% on Windows. The gain comes from parsing and, much more, from track time bounds, which no
longer parse every timestamp.

Two steps got slower: `list(segment.points)` and freeing the point lists, by about 20 ns per
point together on Linux and 37 on Windows. `LatLong` did not get larger: it is 72 bytes on both
(GCC 14), because `main` already stored the time and 1b8881a only exposed it. The cause is the
time-bounds change instead. On `main`, `track.time_bounds()` parses every point's timestamp, and
parsing replaces the stored text with the parsed instant in place. After that, copying a point
copies no string. The branch parses only the earliest and latest timestamp, so every point keeps
its text, and every copy that `list(segment.points)` makes allocates a string (a 20-character
timestamp is too long for `std::string`'s built-in buffer), which the free then releases. The TET
routes, which mostly have no timestamps, show no difference in these steps. Checked directly on
one 6,320-point segment: copying the list costs the same on both builds until
`segment.time_bounds()` is called; after that, `main` drops from 77 to 48 ns per point and the
branch stays at 76.

Constructing a `LatLong` with the new four-argument `__init__` costs nothing measurable (Linux
119 → 122 ns within a 116–126 spread; Windows 157 → 153).

Link-time optimization, as the extension is currently built, makes the upload path slower on
Linux, not faster. Parsing the Sleipnir files takes 186 ns per point with it and 169 without; on
the TET routes it is about even. The known `parse_gpx_time` slowdown barely shows, as expected:
time bounds take 12.6 against 11.8 ns. The C++ `LoadGpx` gain in `build_settings.md` is real at
this commit too, but it does not reach the Python module as built.

**Done.**

- Removed the Linux link-time optimization override from `pyproject.toml`; the wheels are Release
  on every platform. Added a pointer to this item under the decision in `build_settings.md`.

Linux (WSL2, GCC 14.2, Python 3.12.3), `gpx/sleipnir`, 106 files, 1.35 million points. ns per
point, the sum of each file's best of 5 rounds, median of 5 alternated runs. The first column is
`main` as it ships (RelWithDebInfo), for reference; the comparison is between the next two.

| `no_copy` step | main as shipped | main Release+LTO | branch Release+LTO | branch Release |
|---|---:|---:|---:|---:|
| `content.decode` | 6.2 | 5.9 | 6.2 | 6.0 |
| `fastgpx.parse(text)` | 172.3 | 192.6 | 185.7 | **169.1** |
| track time bounds | 72.2 | 64.8 | **12.6** | 11.8 |
| `list(segment.points)` | 45.3 | 45.2 | **57.7** | 59.2 |
| `(lon, lat)` tuples | 71.7 | 69.7 | 68.2 | 68.7 |
| segment bounds, length, time bounds | 30.8 | 30.5 | 30.7 | 31.2 |
| freeing the point lists | 13.3 | 13.2 | **21.4** | 21.5 |
| **total** | 414.8 | 423.6 | **382.6** | **367.6** |

| `current` step | main as shipped | main Release+LTO | branch Release+LTO | branch Release |
|---|---:|---:|---:|---:|
| `fastgpx.parse(text)` | 172.3 | 192.5 | 185.0 | 168.6 |
| track time bounds | 72.1 | 64.6 | 12.6 | 11.8 |
| `list(segment.points)` | 45.2 | 45.7 | 56.6 | 58.4 |
| a new `LatLong` per point | 122.3 | 118.8 | 122.3 | 127.5 |
| tuples from the copies | 75.4 | 72.9 | 71.4 | 71.4 |
| freeing the point lists | 27.0 | 27.3 | 38.8 | 38.5 |
| **total** (all steps) | 553.1 | 559.9 | 525.5 | 512.3 |

Windows (MSVC, Python 3.12.7), same files and method. Release on both sides:

| `gpx/sleipnir` step | main as shipped | main Release | branch Release |
|---|---:|---:|---:|
| `fastgpx.parse(text)` | 503.1 | 438.2 | 397.6 |
| track time bounds | 184.1 | 173.5 | 18.0 |
| `list(segment.points)` | 51.7 | 50.3 | 75.9 |
| freeing the point lists | 14.2 | 14.2 | 25.5 |
| **`no_copy` total** | 885.5 | 808.6 | 647.3 |
| **`current` total** | 1060.8 | 979.0 | 815.3 |

`gpx/TET` (34 files, 1.66 million points, mostly untimed), totals only:

| | main as shipped | main Release(+LTO on Linux) | branch Release+LTO | branch Release |
|---|---:|---:|---:|---:|
| Linux `no_copy` | 328.1 | 331.8 | 326.3 | 324.7 |
| Linux `current` | 461.5 | 467.6 | 461.9 | 470.1 |
| Windows `no_copy` | 708.0 | 664.8 | – | 605.4 |
| Windows `current` | 876.5 | 826.8 | – | 762.4 |

A separate check of `parse(text)` and `load(path)` alone, three alternated runs, agreed: on the
Sleipnir files the branch parses in 161–169 ns per point without link-time optimization and
172–188 with it.

Where the numbers came from. All runs used `benchmark_ingest.py` at 3b35bae (clean), with each
build's wheel installed into its own venv; `corpus_manifest.py verify` passed on both platforms.
The raw output stayed outside the repository, in the session scratchpad, as
`results-linux/linux-<build>-run<N>.json` and `results-win/win-<build>-run<N>.json`. Builds, by
the MD5 of the extension that `run_info` records:

| Build | Commit | Settings | Linux `.so` | Windows `.pyd` |
|---|---|---|---|---|
| main as shipped | f4953bf | its own pyproject: RelWithDebInfo | a1157d9083 | 48865bc7c1 |
| main Release(+LTO) | f4953bf | `-C cmake.build-type=Release`, plus `-C cmake.define.CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON` on Linux | beab7cb2ec | 89a631dbe9 |
| branch Release+LTO | 3b35bae | its own pyproject | a5ec130bef | – |
| branch Release | 3b35bae | its own pyproject; on Linux `-C cmake.define.CMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF` | 0f858e58fb | f1625a60e7 |

Both branch Linux builds still had the override applied, so they configured with
`BUILD_TESTING=ON` (see below). That does not change the extension's compile flags.

**Why.** Production reads no per-point `.time`, so the case for link-time optimization rested on
faster loading, and as the extension is built today it makes loading slower. Dropping it is the
fastest of the builds that ship today, and one build configuration for every platform. It is an
interim step: see the next point.

**Why link-time optimization slows the Python module** (found by the review of this item). The
C++ `profile_load` is faster with it at this commit too, about 1.17× on the Sleipnir files. GCC's
inlining report for the extension shows two differences from the executable (the mechanism is
inferred from the report; the hot instructions were not profiled):

- pugixml is compiled `-fPIC` with default visibility, so in a shared library its functions may
  be replaced at load time, and GCC refuses to inline them into the parse loop (26 misses,
  "function body can be overwritten at link time"). That inlining is what `build_settings.md`
  credits for the gain.
- nanobind compiles the binding code with `-Os`. The copies of libstdc++ helpers the linker keeps
  come from there, and cannot be inlined into the `-O3` parser (10 misses, "target specific option
  mismatch").

Parse time per point on Linux (GCC 14.2), three alternated rounds:

| Build | `gpx/sleipnir` | `gpx/TET` |
|---|---:|---:|
| Release (what this item ships) | 164 | 109–111 |
| Release + link-time optimization | 182–183 | 113–116 |
| nanobind `NOMINSIZE`, no link-time optimization | 154–164 | 105–110 |
| `NOMINSIZE` + link-time optimization | 159–163 | 105–109 |
| `NOMINSIZE` + link-time optimization + `-fno-semantic-interposition` | **130–140** | **86–92** |

With both fixes, the extension's inlining report looks like `profile_load`'s. `NOMINSIZE` alone
also makes `list(points)` plus the free about 10 ns per point faster. That last build is a
separate item: it has to be measured on Windows and over the whole upload path, and to pass the
CLAUDE.md checks.

**Not done.**

- The C++ figures in `build_settings.md` were not re-measured.
- The string copy per point is not fixed. Two ways out: keep the unparsed timestamp without a
  heap allocation (they are short and of known forms), or the bulk coordinate accessor proposed in
  #73, which would let Sleipnir skip the point copies altogether. Either should be its own item.
- In a whole-corpus check, copying and freeing freshly parsed points (before any time bounds) was
  still about 10 ns per point slower on the branch than on `main`. The single-segment check shows
  no such difference. Not explained, and not confirmed.
- The Linux wheels were built locally with Ubuntu's GCC 14.2 and `uv build`, not with
  cibuildwheel in the `manylinux_2_28` image. Linux ARM remains unmeasured.
- The link-time optimization override also turned `BUILD_TESTING` back on in Linux wheel builds:
  a build with the override configured Catch2 and the tests, and a build with the same setting
  passed on the command line did not, and neither does the edited `pyproject.toml`. So the
  override replaced the whole `cmake.define` table instead of adding to it, which is
  scikit-build-core's default for overrides. Removing it fixes that, but any future platform
  override of `cmake.define` has to repeat `BUILD_TESTING` or set
  `inherit.cmake.define = "append"`.
