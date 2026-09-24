# Review decisions: `dev/latlong-time`

A review of the `dev/latlong-time` branch (LatLong and time parsing changes, link-time
optimization for the Linux wheels, and the benchmark write-ups in this folder) turned up a list
of items. They are handled one at a time on `dev/latlong-time-review`. This file records, per
item, what was found, what was done and why, and what was left alone on purpose, so the reasoning
survives into later sessions.

## Summary

Linux figures are WSL2 with GCC 14.2 on `gpx/sleipnir` (106 files, 1.35 million points), ns per
point on the upload path after thomthom/sleipnir#596 (`no_copy`), median of 5 alternated runs.

| Item | Commit | Outcome | Headline numbers |
|---|---|---|---|
| Benchmarks cannot be traced to their files | `3b35bae` | `corpus_manifest.json` lists `gpx/sleipnir` and `gpx/TET` by MD5; `benchmark_corpus.py` records the build and files; `benchmark_ingest.py` replaces the lost upload-path script | – |
| Is the branch a win for the upload path; does link-time optimization help | `b01d107` | Branch is faster than `main` on both platforms. Link-time optimization as built made Linux parsing slower, so the wheels became Release everywhere | Total, `main` → branch built alike: Linux 423.6 → 382.6 (−10%, both Release+LTO), Windows 808.6 → 647.3 (−20%, both Release). Linux parse 186 with LTO, 169 without |
| Link-time optimization that reaches the parser | `7a03e1d` | pugixml and core library built with hidden visibility, `NOMINSIZE` on (except MSVC), LTO back on for Linux. Windows unchanged | Linux parse 167.4 → 136.6 (−18%), total 359.4 → 322.4 (−10%). `manylinux_2_28`: parse 175.1 → 144.8. `parse_gpx_time` 23.6 → 35.0 ns (C++, not on the upload path) |
| Copying a point allocates; `LatLong` equality and hashing | `24b3d2f` | Timestamp text stored inline; equality at microseconds; `LatLong.__hash__`; `time` accepts only `datetime.datetime` | Linux total 338.0 → 307.5 (−9%), `list(points)` 55.2 → 40.1, free 20.9 → 13.3. Windows total 686 → 622 (quiet runs). C++ copy+free of 19,962 points: 525 → 81 µs Linux, 1,489 → 374 µs Windows |
| Docs in line with the branch | `4c6284a` | Stale build types, collection sizes, wrong ratios and the fuzz instructions fixed; `performance.md` gained sections for the last two items | – |
| Unhashable `LatLong`; link-time optimization only where supported; strict `TimeBounds` | `9a39c91` | `LatLong.__hash__` is `None` and `LatLong::Hash()` is gone; `FASTGPX_LTO` asks for link-time optimization and falls back with a warning; `TimeBounds` takes only `datetime.datetime` (behaviour change) | – (GCC 14 wheel flags identical to before) |
| CI fix, not a review item: the wheel workflow failed on 32-bit builds | `efdcdf3` | No more `win32` and `i686` wheels (user-visible: 0.5.0–0.7.0 shipped `win32`) | – |
| Unhashable `TimeBounds` and `Bounds`; tests off in any scikit-build-core build | uncommitted | `TimeBounds.__hash__` and `Bounds.__hash__` are `None` (behaviour change); CMake defaults `BUILD_TESTING` to off when `SKBUILD` is set, and `pyproject.toml` no longer defines it | – (build configuration only) |

Open for the user, most production impact first:

- **Measure Linux ARM, and build a wheel with cibuildwheel itself.** Neither was done. The
  link-time optimization build was checked in the `manylinux_2_28` image; the inline-text change
  was not.
- **Run the wheel workflow on `efdcdf3` or later, and say in the next release notes that 32-bit
  Windows wheels are gone.** See the CI fix section; the fix itself has not run in CI yet.
- ~~Decide whether a mutable `LatLong` should stay hashable.~~ Resolved: unhashable, option (a).
  See the item making `LatLong` unhashable.
- ~~Decide whether the Linux override should force link-time optimization.~~ Resolved: it asks
  for it only where the compiler supports it. See the item making `LatLong`
  unhashable.
- ~~Keep `inherit.cmake.define = "append"` in any future platform override of `cmake.define`.~~
  Resolved: CMake now keeps the tests out of a scikit-build-core build by itself, so an override
  can no longer turn them back on. See the last item.
- **Look at the bulk coordinate accessor (#73).** It would save creating a wrapper per point on
  the upload path; it was not looked at.
- **Re-run the Windows numbers for the inline timestamp storage in a quiet session.** The machine
  was noisy. The two quiet runs show `no_copy` 686 → 622 ns per point, and all 5 run pairs favoured
  the change. But over all 5 runs the median `current` total and parse got slightly worse. Linux
  is unaffected and is the production figure.
- **Revisit link-time optimization on Windows if the Windows upload path starts to matter.** It
  makes that path about 11% faster but polyline work 4–33% slower in C++.
- ~~Decide whether `TimeBounds` should stop accepting `datetime.date` and `datetime.time`.~~
  Resolved: it stops, as a behaviour change. See the item making `LatLong`
  unhashable.
- ~~Decide whether `TimeBounds` and `Bounds` should become unhashable too.~~ Resolved:
  unhashable, as a behaviour change. See the last item.
- **Re-read GCC's inlining report for the shipped build** to confirm how `NOMINSIZE` helps. The
  mechanism is plausible, not verified.
- **Explain the MSVC `polyline::decode` Catch2 figures** (250–330 µs, very noisy), far above the
  139 µs in `build_settings.md`.
- **Re-measure the old figures if they are to be trusted as absolutes:** the C++ figures in
  `build_settings.md`, the 154- and 183-file rows, and the 88 → 65 ns `parse_gpx_time` row. The
  183-file makeup is unconfirmed, and the manifest does not cover Sleipnir's `gpx/tet`.

## Final verification (4c6284a)

Run over the branch as it stood after the docs item. The last two items came later
and record their own checks.

- **Sanitized Clang 18 build** (address and undefined, `FASTGPX_BUILD_FUZZERS=ON`), following
  `Development.md`: all 62 CTest tests pass, which are the 58 Catch2 cases plus the four
  `fuzz_*_corpus` replays. No sanitizer reports.
- **Fuzz runs.** No crashes, leaks or timeouts:

  | Target | Time | Runs |
  |---|---:|---:|
  | `fuzz_gpx` | 6 min | 2.0M |
  | `fuzz_datetime` | 6 min | 9.3M |
  | `fuzz_polyline` | 2 min | 2.3M |
  | `fuzz_polyline_encode` | 2 min | 2.2M |

- **MSVC 19.51:** Catch2 and CTest pass (62/62).
- **Python on Windows:** 200 passed, and the stubs did not drift.
- **Linux wheel** (GCC 14): configures as Release with link-time optimization, `BUILD_TESTING`
  off, `NOMINSIZE` and hidden visibility. Installed into a clean venv, its tests give 197 passed
  and 3 skipped, all for environmental reasons.
- **The fuzz instructions** worked as written once a newer CMake was on PATH. Ubuntu's 3.28 is
  below the project's minimum of 3.30.2, and `Development.md` now says so.

## Decisions made up front

- **The 183-file figure.** `performance.md` and `build_settings.md` quote a 183-file collection
  that nobody can trace. It is recorded as most likely all 145 files of Sleipnir's upload folder,
  duplicates included, plus the 34 TET country routes in `gpx/TET` and the 4 in Sleipnir's
  `gpx/tet` (145 + 34 + 4 = 183). Unconfirmed. Those folders hold 144 unique files, which matches
  neither the 140 nor the 154 quoted elsewhere; 140 does match `gpx/sleipnir` + `gpx/TET`.
- **Multi-run numbers go into `performance.md`**, not into rewritten commit messages. Old commits
  keep their single-run numbers.
- **LatLong equality** will compare at microsecond resolution and gain a value hash (option b),
  provided that costs nothing measurable on the production path. Done in a later item. The hash
  was later dropped for option (a), unhashable; see the item making `LatLong` unhashable.
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

## Item: build the Linux wheels so that link-time optimization reaches the parser

**Found.** The previous item found a build that parsed the Sleipnir files at 130–140 ns per point,
against 164: nanobind's `NOMINSIZE`, link-time optimization and a global
`-fno-semantic-interposition`. That flag is broad. It changes the rules for every function in
every target. What needs it is narrower. Only pugixml and the fastgpx core library were
interposable, because both are static libraries compiled `-fPIC` with default visibility. Neither
is meant to be seen outside the extension, yet the extension exported 734 symbols, nearly all of
them pugixml and fastgpx internals.

Hidden visibility on those two libraries makes their functions non-interposable, which is what
the flag did for them. With it, GCC 14 produced a byte-identical extension whether or not
`-fno-semantic-interposition` was added on top (MD5 `c40607a1a9` both ways). So the flag adds
nothing once the visibility is right. Built with Ubuntu's GCC 14, the extension now exports 29
symbols. From the `manylinux_2_28` image it exports 356 (1,038 before): libstdc++ pieces that
gcc-toolset links in statically, none from pugixml or fastgpx.

On Windows, link-time optimization makes the upload path about 11% faster, but polyline encoding
and decoding slower again, from C++ and from Python. `NOMINSIZE` alone made parsing slightly
slower there.

**Done.**

- `src/cpp/CMakeLists.txt` builds `pugixml-static` and `fastgpx-static` with
  `CXX_VISIBILITY_PRESET hidden` and `VISIBILITY_INLINES_HIDDEN`. This has no effect with MSVC.
- A new option, `FASTGPX_NOMINSIZE`, passes `NOMINSIZE` to `nanobind_add_module`. It is on by
  default, except with MSVC.
- `pyproject.toml` turns link-time optimization back on for Linux through a platform override
  with `inherit.cmake.define = "append"`. The configure output of the wheel build shows
  `BUILD_TESTING` still off and Catch2 not fetched. Windows wheels are built as before.

Linux (WSL2, GCC 14.2 pinned with `CC=gcc-14 CXX=g++-14`, Python 3.12.3). ns per point, the sum
of each file's best of 5 rounds, median of 5 alternated runs. "Hidden" is the visibility change.

| `gpx/sleipnir`, `no_copy` step | HEAD (Release) | hidden + `NOMINSIZE` + LTO (**shipped**) | hidden + LTO | hidden + `NOMINSIZE` |
|---|---:|---:|---:|---:|
| `content.decode` | 5.4 | 5.3 | 5.6 | 5.5 |
| `fastgpx.parse(text)` | 167.4 | **136.6** | 157.3 | 154.3 |
| track time bounds | 11.7 | 9.8 | 10.6 | 10.4 |
| `list(segment.points)` | 58.2 | 50.4 | 56.9 | 50.8 |
| `(lon, lat)` tuples | 67.0 | 68.6 | 68.1 | 69.2 |
| segment bounds, length, time bounds | 30.5 | 30.1 | 30.3 | 30.6 |
| freeing the point lists | 20.3 | 19.2 | 19.8 | 19.5 |
| **total** | 359.4 | **322.4** | 348.4 | 341.0 |

| Totals | HEAD | shipped | hidden + LTO | hidden + `NOMINSIZE` |
|---|---:|---:|---:|---:|
| `gpx/sleipnir`, `current` | 497.6 | **456.7** | 485.6 | 477.3 |
| `gpx/TET`, `no_copy` | 320.5 | **301.6** | 307.9 | 317.3 |
| `gpx/TET`, `current` | 455.9 | **437.0** | 444.9 | 449.4 |
| `gpx/sleipnir` parse, range over the 5 runs | 165–174 | 136–141 | 155–160 | 151–158 |

On Sleipnir's files, the path production is moving to is 10% faster in total, and parsing is 18%
faster. `NOMINSIZE` accounts for about 26 ns per point of that. Link-time optimization gains
little without it, and it gains less without link-time optimization.

Windows (MSVC 19.51, Python 3.12.7), same method. Hidden visibility changes nothing with MSVC, so
the shipped Windows wheel has the same flags as HEAD:

| Step or total | HEAD (**shipped**) | `NOMINSIZE` | LTO | `NOMINSIZE` + LTO |
|---|---:|---:|---:|---:|
| `gpx/sleipnir` `fastgpx.parse(text)` | 394.6 | 406.4 | 334.8 | 325.9 |
| `gpx/sleipnir`, `no_copy` total | 652.0 | 659.8 | 592.2 | 580.4 |
| `gpx/sleipnir`, `current` total | 822.8 | 837.5 | 759.2 | 751.9 |
| `gpx/TET`, `no_copy` total | 605.0 | 603.7 | 592.0 | 567.8 |
| `gpx/TET`, `current` total | 767.2 | 771.2 | 756.6 | 733.4 |
| `polyline.encode` from Python, ns/point | 10.1 | 10.2 | 10.9 | 11.1 |
| `polyline.decode` from Python, ns/point | 60 | 59 | 63 | 64 |

The Python polyline figures cover every segment of `gpx/sleipnir`, best of 7, over three
alternated runs. The runs agreed within 0.2 ns for encoding and 3 ns for decoding.

Catch2 benchmarks, median of the means over 5 alternated runs, 50 samples each:

| Benchmark | Linux HEAD | Linux hidden | Linux hidden + LTO | Windows Release | Windows LTO |
|---|---:|---:|---:|---:|---:|
| `polyline::encode` 10k, precision 5 | 72.5 µs | 73.2 µs | 67.7 µs | 75.3 µs | 78.7 µs |
| `polyline::encode` 10k, precision 6 | 79.4 µs | 73.7 µs | 74.2 µs | 84.3 µs | 95.9 µs |
| `polyline::decode` 10k, precision 5 | 62.5 µs | 57.3 µs | 59.3 µs | 292 µs | 309 µs |
| `polyline::decode` 10k, precision 6 | 67.8 µs | 65.8 µs | 67.4 µs | 247 µs | 329 µs |
| `parse_gpx_time` | 23.6 ns | 23.7 ns | 35.0 ns | – | – |

From Python on Linux, polyline encoding stayed at 8.3 ns per point, and decoding went from
41–43 to 40 ns. The `parse_gpx_time` cost of link-time optimization is the one described in
`build_settings.md`. It does not reach the upload path, where time bounds got faster (11.7 to
9.8 ns per point).

Size, in bytes. The Linux extension and wheel got smaller, because the hidden symbols are no
longer exported. `NOMINSIZE` costs about 110 KB of the extension:

| Build | Linux `.so` | Linux wheel | Windows `.pyd` | Windows wheel |
|---|---:|---:|---:|---:|
| HEAD | 572,504 | 233,731 | 444,416 | 201,588 |
| shipped | 377,104 | 173,184 | 444,416 | 201,589 |
| hidden + LTO, no `NOMINSIZE` | 266,544 | 122,513 | – | – |
| hidden + `NOMINSIZE`, no LTO | 598,616 | 259,385 | – | – |
| `NOMINSIZE` | – | – | 455,680 | 204,529 |
| LTO | – | – | 430,592 | 198,191 |
| `NOMINSIZE` + LTO | – | – | 441,344 | 199,772 |

Where the numbers came from. Wheels were built with `uv build --wheel` from HEAD (b01d107) and
from the working tree with this change, each installed into its own venv. The variants were built
from the same tree, with `-C cmake.define.…` on the command line. `corpus_manifest.py verify`
passed on both platforms. The raw output stayed outside the repository: on Linux in
`~/fastgpx-review/v2/results/ingest/linux-<build>-run<N>.json`, on Windows in the session
scratchpad as `win/results/ingest/win-<build>-run<N>.json`. Builds, by the MD5 of the extension:

| Build | Settings | Linux `.so` | Windows `.pyd` |
|---|---|---|---|
| HEAD | its own pyproject: Release | 0f858e58fb (same as "branch Release" above) | fac8f99d8c |
| shipped | this change's pyproject | c40607a1a9 | dc616e57f6 |
| shipped + `-fno-semantic-interposition` | `CMAKE_CXX_FLAGS` | c40607a1a9 | – |
| hidden + LTO | `FASTGPX_NOMINSIZE=OFF` | 8862ed8c62 | – |
| hidden + `NOMINSIZE` | `CMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF` | 55f494f9b9 | – |
| `NOMINSIZE` | `FASTGPX_NOMINSIZE=ON` | – | 72caad6b2b |
| LTO | `FASTGPX_NOMINSIZE=OFF`, `CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON` | – | a46eb17317 |
| `NOMINSIZE` + LTO | `FASTGPX_NOMINSIZE=ON`, `CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON` | – | 490419b881 |

The Windows variants were built before `FASTGPX_NOMINSIZE` defaulted to off with MSVC, hence the
explicit settings. The shipped Windows build has the same flags and size as HEAD. Its MD5 differs
because MSVC writes a timestamp into the binary.

Verified:

- The Python test suite with `-m "not wip"`, on the shipped wheels: 173 passed and 1 skipped on
  Linux, 174 passed on Windows.
- The Catch2 suite with MSVC and with GCC 14.
- The sanitized Clang 18 fuzz build configures and builds with the hidden visibility, all four
  `fuzz_*_corpus` replays pass, and a 2-minute `fuzz_gpx` run found nothing.
- The stubs did not change.

**Why.** Hidden visibility is the most targeted fix:

- It changes only the two libraries whose symbols were interposable.
- It states what is true of them: nothing outside the extension uses them.
- It means the same to GCC and Clang.

A global `-fno-semantic-interposition` would also reach nanobind and the extension's own code,
and Clang already assumes it for most code, so the flag would behave differently between the two
compilers. Setting the flag on the two targets only would work with GCC, but would leave 700
symbols exported for no reason. Visibility also shrinks the extension, which more than pays for
`NOMINSIZE`.

Windows stays as it was, because it is used for development and this change was not to regress
it. Link-time optimization there is a trade. The upload path gets about 11% faster, but polyline
work gets slower: 4–33% in C++ and 5–8% from Python. That is the same trade that kept it off
before. It is worth revisiting if the Windows upload path ever matters more.

**Not done.**

- Not built with cibuildwheel itself. The reviewer built both wheels in the same
  `manylinux_2_28` image (GCC 14.2.1, gcc-toolset-14) with the same settings and `auditwheel
  repair`: `gpx/sleipnir` `no_copy` parse 175.1 → 144.8 and total 392.1 → 347.9 ns per point,
  median of 5 alternated runs, so the gain holds in production's toolchain. Linux ARM remains
  unmeasured.
- With link-time optimization forced on, a Linux build from the sdist with a compiler CMake
  cannot do it for (for example Clang without `llvm-ar`) stops at configure. The earlier
  link-time optimization commit had the same exposure; not addressed. *(Superseded: addressed in
  the item making `LatLong` unhashable, which also found that such a build fails while building,
  not at configure.)*
- Clang was checked only in the sanitized fuzz build, not with link-time optimization. The
  Linux wheels are built with GCC.
- GCC's inlining report was not re-read for the new build, so the stated `NOMINSIZE` mechanism
  (the `-Os` copies of shared inline functions win at link time) is plausible, not verified. The byte-identical extension with and
  without `-fno-semantic-interposition` stands in for it.
- The MSVC `polyline::decode` Catch2 figures (250–330 µs, with a standard deviation near half the
  mean) are far above the 139 µs in `build_settings.md`. Both builds are noisy there and the gap
  is not explained. The comparison between the two builds stands; the absolute figures do not.
- The fuzzing instructions in `Development.md` configure with `BUILD_TESTING=OFF`, which leaves
  no `fuzz_*_corpus` tests for `ctest` to run. The replays here needed `BUILD_TESTING=ON`. The
  instructions were not changed.

## Item: copying a point allocates, and `LatLong` equality and hashing

**Found.** Since 9b94af8, track time bounds parse only the earliest and latest timestamp, so every
point keeps its `<time>` text. `TimePoint` held that text in a `std::string`, and a 20- or
24-character timestamp is too long for the string's built-in buffer. So every point copy that
`list(segment.points)` makes allocated, and freeing the list released it. Parsing allocated one
per point as well. The previous item measured the cost at about 20 ns per point on Linux and 37 on
Windows.

Two smaller problems sat next to it. `LatLong` equality compared instants at the clock's
resolution, which is 100 ns with MSVC and 1 ns with libstdc++, while `LatLong.time` only carries
microseconds. So a point rebuilt from its own `.time` could compare unequal to it. And the Python
`LatLong` compared by value but hashed by identity. Separately, the `time` setter and the
`LatLong(...)` time argument accepted `datetime.date` and `datetime.time`, which the conversion
turned into midnight or a time on 1970-01-01.

**Done.**

- `TimePoint` stores unparsed text of up to 38 characters inside the object. That covers every
  form `parse_gpx_time` accepts short of a very long fraction. Longer text goes on the heap. The
  object is still 40 bytes (a `static_assert` checks it), so `LatLong` stays 72 bytes. The storage
  is a byte buffer read and written with `memcpy`, which lets the length and the kind tag sit where
  a union's tail padding would be. Copying inline text or a parsed instant is a fixed-size copy.
- `raw()` now returns `std::optional<std::string_view>` instead of `const std::string*`. The time
  bounds fast path, equality and `__repr__` use it as before.
- Equality compares instants floored to microseconds, which is how the conversion to `datetime`
  truncates, before 1970 too. Identical text is still equal without parsing. Unparseable text is
  equal only to identical text.
- `LatLong::Hash()` is consistent with that equality. It hashes the coordinates and elevation, with
  -0.0 hashed as 0.0 and every NaN alike. For the time it hashes the microsecond instant, or the
  text when the text does not parse. Python's `LatLong.__hash__` calls it. It parses on each call
  and stores nothing, so loading pays nothing for it.
- `LatLong(...)` and the `time` setter take a new `utc_datetime` argument type. Its caster accepts
  only `datetime.datetime` and its subclasses; anything else is a `TypeError`. `TimeBounds` keeps
  the looser conversion, because it is older than this branch.
- The stubs changed only for the bindings: the two `time` signatures and the new `__hash__`.
- Tests. C++: copies and moves of all three storage kinds, microsecond equality (`.0000001Z` equals
  `.0000009Z`, including before 1970), hash against equality, a `<time>` too long for the inline
  buffer, and an offset that crosses a year boundary. Python: a copied point keeps its time, a
  point rebuilt from `p.time` equals `p` with the same hash, `len({p, q}) == 1` for a parsed and a
  constructed point, -0.0, NaN, unparseable text, the setter and constructor rejecting `date`,
  `time`, `str` and `int`, the year-boundary offset, and whitespace-padded coordinates in a
  pretty-printed document.
- `fuzz_gpx` now copies every point before the time bounds, parses the copy's time, and asserts
  that the copy still equals the original and that equal points hash alike. It has to come first,
  because time bounds throw on the first malformed `<time>`.
- A new Catch2 benchmark, `[!benchmark][latlong]`, copies and frees the 19,962 points of the TopCamp
  file.

Linux (WSL2, GCC 14.2 pinned with `CC=gcc-14 CXX=g++-14`, Python 3.12.3, the wheel configuration
of `pyproject.toml`: Release, link-time optimization, `NOMINSIZE`). `gpx/sleipnir`, 106 files,
1.35 million points. ns per point, the sum of each file's best of 5 rounds, median of 5 alternated
runs, with the range over the runs:

| `no_copy` step | HEAD | this change |
|---|---:|---:|
| `content.decode` | 6.8 | 6.4 |
| `fastgpx.parse(text)` | 142.7 (141.8–145.7) | **135.7** (134.9–136.5) |
| track time bounds | 10.1 | 10.1 |
| `list(segment.points)` | 55.2 (53.9–56.6) | **40.1** (38.9–41.2) |
| `(lon, lat)` tuples | 70.2 | 70.1 |
| segment bounds, length, time bounds | 31.3 | 31.0 |
| freeing the point lists | 20.9 (19.6–21.4) | **13.3** (13.0–13.5) |
| **total** | 338.0 (332.8–344.9) | **307.5** (304.5–310.1) |

| `current` | HEAD | this change |
|---|---:|---:|
| `list(segment.points)` | 53.8 | 41.1 |
| freeing the point lists | 38.5 | 27.9 |
| **total** | 483.9 | 452.8 |

On the path production is moving to, the total drops by 9%. Copying and freeing are back at or
below `main` as shipped (45.3 and 13.3 in the second item). Parsing is 7 ns per point faster,
because it no longer allocates the string either. Time bounds did not change.

Windows (MSVC 19.51, Python 3.12.7, Release wheels). The machine was noisy through the whole
session: `parse` ranged from 395 to 520 ns per point on the same build. The load changed between
runs and hit both builds of a pair alike. So the table gives the two quiet runs (3 and 4) as well
as the median of all 5:

| `no_copy` step | HEAD, runs 3–4 | this change, runs 3–4 | HEAD, median of 5 | this change, median of 5 |
|---|---:|---:|---:|---:|
| `fastgpx.parse(text)` | 413–416 | 395–397 | 451.2 | 483.2 |
| track time bounds | 18.8–19.0 | 14.4 | 20.7 | 15.3 |
| `list(segment.points)` | 87 | 57–58 | 98.7 | 65.0 |
| freeing the point lists | 26–27 | 15 | 30.3 | 17.4 |
| **total** | 686–687 | **622–623** | 748.4 | 737.6 |
| `current` total | 869–875 | 816 | 962.9 | 974.8 |

In each of the 5 pairs, the `no_copy` total was lower with this change, by 62, 76, 65, 62 and 11 ns
per point.

Catch2, median of the means over 5 alternated runs, 50 samples each:

| Benchmark | Linux HEAD | Linux change | Windows HEAD | Windows change |
|---|---:|---:|---:|---:|
| copy and free 19,962 points (new) | 525 µs | **81 µs** | 1,489 µs | **374 µs** |
| `parse_gpx_time` | 22 ns | 22 ns | 69 ns | 65 ns |
| `LoadGpx` TopCamp 20240518 | 3.51 ms | 3.22 ms | 15.5 ms | 11.8 ms |
| `LoadGpx` TopCamp 20240520 | 4.49 ms | 3.88 ms | 15.2 ms | 14.6 ms |
| `Segment::GetTimeBounds` | 53.9 µs | 57.7 µs | 159 µs | 120 µs |

The Linux `GetTimeBounds` median is 7% higher, but the ranges overlap (53.5–59.6 against
53.4–60.0), and the same step from Python did not change (10.1 ns per point on both). The Windows
Catch2 figures come from the same noisy session.

Where the numbers came from. HEAD is 7a03e1d. The candidate is that commit plus this change,
uncommitted. Wheels were built with `uv build --wheel`, each installed into its own venv.
`benchmark_ingest.py` ran over `gpx/sleipnir` on each platform's own file system, and
`corpus_manifest.py verify` passed on both. The raw output stayed outside the repository: on Linux
in `~/fastgpx-review/v3/results/{ingest,c2}/linux-<build>-run<N>.*`, on Windows in the session
scratchpad as `v3win/{ingest,c2}/win-<build>-run<N>.*`. Extensions by MD5:

| Build | Linux `.so` | Windows `.pyd` |
|---|---|---|
| HEAD | c40607a1a9 (same as "shipped" in the previous item) | 1476727abe |
| this change | 5d7562f1c4 | 26032136cb |

Verified:

- The Catch2 suite with MSVC and with GCC 14.
- The Python suite on the new wheels, with no marker filter: 200 passed on Windows; 199 passed and
  1 skipped on Linux.
- The stubs changed only as described above.
- Sanitizer and fuzz verification is deferred to the final pass over the branch, after the last
  item. They had already been run on this change before that was decided. The sanitized Clang 18
  build (`FASTGPX_BUILD_FUZZERS=ON`, `BUILD_TESTING=ON`) passed the Catch2 suite, whose tests link
  the sanitized core library, and all four `fuzz_*_corpus` replays. A 3-minute `fuzz_gpx` run with
  the new checks and a 2-minute `fuzz_datetime` run found nothing.

The review of this item fixed two edge cases and added tests for them:
- `memcpy` from the null pointer of an empty `string_view`, which is undefined even for zero
  bytes. It is reachable through the public `TimePoint` constructor.
- A missing `catch (...)` in the `noexcept` converter for a point's time.

The new tests cover text at the 38/39-character edge between inline and heap storage, a failed
parse of heap text, and self-move. The reviewer then passed the Catch2 suite on MSVC, GCC 14 and
sanitized Clang 18, all four corpus replays, and a 90-second `fuzz_gpx` run.

**Open for the user: a mutable object is now hashable.** *(Superseded: the user chose option (a),
and `LatLong` is unhashable since the item "make `LatLong` unhashable, …" below. Kept for the
record.)* `latitude`, `longitude`, `elevation` and
`time` can all be assigned. A point changed while it is in a set or used as a dict key is no
longer found there. This is the trade-off of option (b), which was chosen up front. `LatLongList`
went the other way (`__hash__ = None`, like `list`). The alternative is to drop `__hash__` and make
`LatLong` unhashable, which is option (a). Kept as (b), as decided; revisit if it bites.

**Why.** Storing the text inline removes the allocation from all three places that paid for it:
parse, copy and free. The time-bounds gain depends on points keeping their text, and they still do.
The heap fallback keeps any text exactly, so odd or long timestamps behave as before. Microsecond
equality matches what Python can see, and it gives the same answer on every platform. The hash had
to follow equality, and computing it only on request keeps it off the load path.

**Not done.**

- The bulk coordinate accessor proposed in #73 was not looked at. With copies this cheap it matters
  less, but it would still save creating a wrapper per point.
- Linux ARM remains unmeasured, and the wheels were not built in the `manylinux_2_28` image.
- `TimeBounds` still accepts `datetime.date` and `datetime.time`. Changing that would break an API
  that is older than this branch. *(Superseded: changed in the item making `LatLong` unhashable,
  at the user's decision.)*
- The unexplained 10 ns per point from the second item was not re-checked on its own: copying
  freshly parsed points was slower on the branch than on `main`. Copying and freeing are now at or
  below `main`'s figures, so it no longer shows.

## Item: bring the benchmark and build docs in line with the branch

**Found.** Several write-ups still said the wheels were RelWithDebInfo, or that nothing had been
changed yet (`build_settings.md`, `load_profile.md`, the machine table of `performance.md`). The
collection sizes quoted (154, 140 and 183 files) were not related to the manifest. A few figures
in `performance.md` were wrong or unsupported: the combined table credited `9b94af8` with a
`parse_gpx_time` gain (1.35× against the 1.28× measured for `8c87bf1`), 4.57 / 0.71 was given as
6.5× (it is 6.4×) and 16.1 / 12.6 as 1.27× (1.28×), the `LatLong ==` note left out the 475 ns
text-against-instant case, and the reason given for the trim's missing Linux gain ignored
`load_profile.md`'s 13%. The Clang fuzz instructions in `Development.md` configured with
`BUILD_TESTING=OFF`; configuring that way registers no tests at all (`ctest -N`: 0). The "Format as
a python datetime string" comment in `python_fastgpx.cpp` still sat above the time conversion
helpers.

**Done.** Nothing was re-measured; every figure comes from this file or the existing docs.

- Build type: the older docs now say RelWithDebInfo was the setting at the time and point to the
  current one (Release everywhere; on Linux, link-time optimization with hidden visibility and
  `NOMINSIZE`). `performance.md` says per section which build type applies.
- Collections: `performance.md` and `build_settings.md` label the 154- and 183-file rows as
  untraceable, give the most likely makeup of the 183, say to compare them by ratio only, and point
  to `corpus_manifest.json` (`gpx/sleipnir` 106 files, 1.35 million points; `gpx/TET` 34 files,
  1.66 million). `ingest_profile.md` already related its 140 files to the manifest; it now also says
  its Linux wheel was the earlier link-time optimization build.
- `performance.md`: the ratios fixed; the 69 → 65 ns gap footnoted as between sessions, not from
  `9b94af8`; the 475 ns case added, with a pointer to the later microsecond equality; the trim note
  now names the `-O2` profile against the `-O3` measurement as the likely reason, unconfirmed; the
  `parse_gpx_time` pairs (24 → 35, 24 → 36, 25 → 36, 23.6 → 35.0) noted as different sessions, here
  and in `build_settings.md`. Two sections added, for the build change and for inline timestamp
  text, with numbers from the items above, and a note that multi-run numbers live in this file,
  not in old commit messages (decision (b) up front).
- `Development.md`: the Clang fuzz build leaves `BUILD_TESTING` on and runs `ctest`.
- The comment moved above `FormatTimePointAsDateTime`.

Verified: Sphinx with `-W --keep-going --fresh-env` (`sphinx-build -M html docs/source docs/build`,
what `make html` runs) builds clean on Windows against the local extension, which is the current
one (its `time` setter rejects a `date`). A Windows configure with `BUILD_TESTING=OFF` and
`FASTGPX_BUILD_FUZZERS=ON` lists 0 tests.

**Not done.**

- The Clang instructions as now written were not run; the previous items did run the replays with
  `BUILD_TESTING=ON`.
- The 88 → 65 ns row was footnoted rather than changed, since which session is right is unknown.
- The Surface rows in `performance.md` were left as they are, from the commit messages.

## Item: make `LatLong` unhashable, ask for link-time optimization only where supported, and a strict `TimeBounds`

Three follow-ups the user decided from the open points above.

**Found.**

- `LatLong` compared by value and was hashable, while all four of its fields can be assigned. A
  point changed while it sat in a set or served as a dict key was lost there. Python's convention
  for a mutable object with value equality is `__hash__ = None`, as for `list` and, already,
  `LatLongList`. Nothing but the Python `__hash__` and the checks written for it used
  `LatLong::Hash()`. `fuzz_gpx` asserted it agreed with equality; the Catch2 tests tested it.
- The Linux override set `CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON` outright. The earlier note said a
  compiler CMake cannot do link-time optimization for makes the sdist build stop at configure.
  Checked here, that is not quite what happens. For Clang, CMake always assumes support, so
  configure passes. With Ubuntu's Clang 18 the build then works, because CMake finds `llvm-ar-18`.
  With the archiver missing (simulated by pointing it at a path that does not exist), the build
  fails at the first static library, pugixml. Either way the sdist build failed.
- `TimeBounds(...)`, its `start_time` and `end_time` setters and `add(...)` accepted
  `datetime.date` (as midnight) and `datetime.time` (on 1970-01-01), which the previous item had
  already ruled out for `LatLong.time`.

**Done.**

- `LatLong.__hash__` is `None`, set the same way as for `LatLongList`. `hash(p)`, `set([p])` and
  `{p: …}` raise `TypeError`; equality is unchanged. **Behaviour change against `main` too:** there,
  `LatLong` compared by value but kept the identity hash, so sets and dict keys of points worked
  (by identity). They now raise `TypeError`. Sleipnir does not hash points. `LatLong::Hash()`, `TimePoint::Hash()` and
  their helpers are removed, with their Catch2 checks and the hash assertion in `fuzz_gpx`. The
  Catch2 test "LatLong hash is consistent with equality" became "LatLong equality in edge cases".
  It keeps the equality checks and turns the "different points differ" hash checks into
  inequality checks. The Python hash tests became equality tests or went; one new test covers
  unhashability.
- A new CMake option, `FASTGPX_LTO` (default off), asks for link-time optimization. With it on,
  `check_ipo_supported` decides. If that passes, `CMAKE_INTERPROCEDURAL_OPTIMIZATION` is set before
  any target is defined, so it reaches pugixml, nanobind and the core library as before. If it
  fails, CMake prints a warning with the reason and builds without. The Linux override now sets
  `FASTGPX_LTO=ON`. `inherit.cmake.define = "append"` and `BUILD_TESTING=false` are unchanged.
  A value given for `CMAKE_INTERPROCEDURAL_OPTIMIZATION` takes precedence over `FASTGPX_LTO`, and
  configure says so. The review found that an explicit `OFF` was ignored and an explicit `ON` was
  reported as `FASTGPX_LTO: OFF`. After the fix, a GCC 14 configure without the Python module gives
  the following:

  | Setting | Compile commands with `-flto` |
  |---|---:|
  | `FASTGPX_LTO=ON` | 6 of 6 |
  | the same, plus `CMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF` | 0 of 6 |
  | `CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON` alone | 6 of 6 |
  | default | 0 of 6 |
- The UTC `system_clock::time_point` caster now accepts only `datetime.datetime` and its
  subclasses; anything else is a `TypeError`. That makes the separate `utc_datetime` type from the
  previous item redundant, so it is gone and `LatLong` uses the same caster again. This covers the
  `TimeBounds` constructor, both setters and `add(datetime)`. **Behaviour change:** code that passed
  a `date` or `time` to `TimeBounds` now gets a `TypeError`. That API is older than this branch.
- The stubs changed only for the bindings: the four `TimeBounds` signatures lose
  `datetime.date | datetime.time`, and `LatLong` has `__hash__: None = None` instead of
  `def __hash__`.

Linux wheel (WSL2, GCC 14.2 pinned with `CC=gcc-14 CXX=g++-14`, `uv build --wheel` with the
scikit-build-core that uv installs, CMake 4.4.3). Built from the previous commit (`35bb46d`) and
from this change:

- Configure passes `-DBUILD_TESTING=FALSE -DFASTGPX_LTO=ON` and prints `FASTGPX_LTO: ON,
  link-time optimization enabled`.
- The 19 build commands (`ninja -t commands`, paths normalised) are identical between the two,
  including `-flto=auto -fno-fat-lto-objects` on the 14 compile and link lines that had them. The
  extension's MD5 differs because the code changed (removed hash, stricter caster). With the flags
  identical, the benchmarks were not re-run.

Fallback, Clang 18 building from the sdist (`uv build`), with `CMAKE_C_COMPILER_AR` and
`CMAKE_CXX_COMPILER_AR` pointed at `/nonexistent/llvm-ar`:

| Tree | Configure | Build |
|---|---|---|
| this change | warning `FASTGPX_LTO is ON, but link-time optimization is not supported with Clang 18.1.3; building without it`, followed by the failed check's output | succeeds; no `-flto` in any build command |
| `35bb46d` | passes | fails archiving `libpugixml.a` (`/nonexistent/llvm-ar: not found`) |

The same Clang with its archiver found builds this change with `-flto=thin`.

Verified:

- MSVC 19.51: Catch2 and the corpus replays through CTest, 62/62.
- Python on Windows, `uv run --reinstall-package fastgpx pytest`: 205 passed.
- Sphinx with `-W --keep-going --fresh-env` builds clean. No docstrings changed.
- Sanitized Clang 18 build with `FASTGPX_BUILD_FUZZERS=ON`: all 62 CTest tests pass (Catch2 and the four
  `fuzz_*_corpus` replays, including `fuzz_gpx`), with no sanitizer reports. A 2-minute
  `fuzz_gpx` run: 960k runs, no crashes, leaks or timeouts.

Raw logs are outside the repository, in `~/fastgpx-review/v5/` (`build-{prev,cand}-gcc.log`,
`build-{prev,cand}-noar.log`, `build-cand-clang.log`, `cmds-{prev,cand}.txt`, `fuzz-*.log`).

**Why.** An unhashable `LatLong` cannot be silently lost in a set or dict, and it matches `list`
and `LatLongList`. Keeping `LatLong::Hash()` would mean maintaining a function, its tests and a
fuzz check for something nothing calls. Asking for link-time optimization rather than forcing it
keeps the production wheel as it was, while a build from the sdist with an unusual toolchain now
succeeds. Making the one caster strict, rather than adding `utc_datetime` to `TimeBounds`, leaves
a single rule for every `datetime` argument in the module.

**Not done.**

- No wheel was built with cibuildwheel or in the `manylinux_2_28` image. The configure and flag
  checks there should match GCC 14 on Ubuntu, but that is inferred.
- The no-archiver case is a simulation. A real toolchain without link-time optimization support
  was not tried.
- Linux ARM remains unmeasured.

## CI fix: no 32-bit wheels (`efdcdf3`)

Not a review item; it came up when the wheel workflow was run on this branch.

**Found.**

- The wheel workflow (run 36051117010, on `8722e06`) failed building `cp312-win32`: the build
  environment could not install `nanobind-backend>=1.0`. The other three jobs were cancelled by
  the failed one, so they never got as far as building.
- `nanobind-backend` 1.0.0 on PyPI has no sdist, only wheels. Those are per Python version
  (cp310–cp315) for `win_amd64`, `win_arm64`, `manylinux` x86_64 and aarch64, riscv64 and macOS.
  There is nothing for `win32`, `i686`, `musllinux` or `armv7l`.
- `main`'s wheel workflow is broken the same way since split mode landed there (`d1a2315`,
  2026-09-09). It only runs by hand, and it last ran on `main` on 2026-01-18, for 0.7.0.
- fastgpx 0.5.0 to 0.7.0 on PyPI each ship a `cp312-abi3-win32` wheel. No release had `i686`.

**Done.** `CIBW_SKIP` also skips `*-win32` and `*_i686`. What remains is cp312 on `manylinux_2_28`
x86_64 and aarch64, Windows AMD64 and Windows ARM64, and `nanobind-backend` has a cp312 wheel for
each. **User-visible:** the next release has no 32-bit Windows wheel. On 32-bit Windows pip falls
back to the fastgpx sdist, and that cannot install `nanobind-backend` either, so fastgpx stops
installing there at all.

**Not done.** The workflow has not been run on `efdcdf3`, so the four remaining builds are
unverified. They are expected to work because the same four built for 0.7.0 and each has a backend
wheel, but that is inferred.

## Item: unhashable `TimeBounds` and `Bounds`, and tests off in any scikit-build-core build

Two follow-ups the user decided from the open points above.

**Found.**

- `TimeBounds` and `Bounds` compared by value but kept Python's identity hash, so
  `TimeBounds() == TimeBounds()` was true while a set held both. Both are mutable: `TimeBounds` has
  `start_time` and `end_time` setters and `add()`, `Bounds` has `min` and `max`, the four gpxpy
  `min_`/`max_` latitude and longitude setters, and `add()`. This predates the branch.
- No other bound class has the same mismatch. `Segment`, `Track` and `Gpx` define no `__eq__`, so
  they compare and hash by identity, which agrees. `SegmentList` and `TrackList` define no `__eq__`
  either (their `in`, `count` and `index` compare identity on purpose). `LatLongList` and, since
  the previous item, `LatLong` are already unhashable. `polyline.Precision` is an enum.
- `pyproject.toml` kept the tests out of the wheel with `BUILD_TESTING = false` in its
  `cmake.define` table. That was the only entry in the table. An override of `cmake.define`
  replaces the table unless it says `inherit.cmake.define = "append"`, and once that dropped
  `BUILD_TESTING` and turned the tests back on in the Linux wheels. scikit-build-core (1.0.3 in
  the WSL build) writes `set(SKBUILD 2 CACHE STRING "" FORCE)` into its initial cache file, so
  CMake can tell it is building a package.

**Done.**

- `TimeBounds.__hash__` and `Bounds.__hash__` are `None`, set the same way as for `LatLong`.
  `hash(x)`, `set([x])` and `{x: …}` raise `TypeError`; equality is unchanged. **Behaviour change
  against `main`:** sets and dict keys of these types used to work, by identity, and now raise
  `TypeError`. Sleipnir does not hash them. It gets them from `bounds()` and `time_bounds()`,
  merges `Bounds` with `add()`, and reads their fields; nothing puts them in a set, uses them as
  dict keys or passes them to a cache. The two test files each gained an unhashability test. The
  stubs changed only by `__hash__: None = None` on the two classes.
- `CMakeLists.txt` defaults `BUILD_TESTING` to off when `SKBUILD` is set and `BUILD_TESTING` is not
  already defined, before `include(CTest)` would default it to on. It also prints `BUILD_TESTING`
  at configure. An explicit `-DBUILD_TESTING=ON` still wins, and plain CMake builds are unchanged.
- `BUILD_TESTING = false` is removed from `pyproject.toml`, which leaves the base `cmake.define`
  table empty and a comment in its place. Keeping it as a second guard would leave two places
  that decide the same thing, and a wheel configure would no longer show whether the CMake
  default works on its own.
- `inherit.cmake.define = "append"` stays on the Linux override. Nothing depends on it now, since
  the base table is empty, but it costs nothing and carries any define added there later. Its
  comment says so instead of warning about `BUILD_TESTING`.

Checked in WSL2 with GCC 14.2 (`CC=gcc-14 CXX=g++-14`) and CMake 4.4.3, on a copy of the working
tree:

| Build | Defines passed to CMake | `BUILD_TESTING` | Catch2 fetched |
|---|---|---|---|
| `uv build --wheel` | `-DFASTGPX_LTO=ON` (link-time optimization enabled) | OFF | no |
| the same, with the old base `BUILD_TESTING = false` restored and `inherit` removed (temporary) | `-DFASTGPX_LTO=ON`; the base define is dropped, as before | OFF | no |
| `uv build --wheel -C cmake.define.BUILD_TESTING=ON` | `-DFASTGPX_LTO=ON -DBUILD_TESTING=ON` | ON | yes, `fastgpx_test` built |
| plain `cmake` configure | none | ON | yes |
| `cmake` configure with `-DSKBUILD=2` | none | OFF | no |

Verified:

- Windows editable build, reconfigured from scratch (`CMakeCache.txt` removed): configure prints
  `BUILD_TESTING: OFF` with no `BUILD_TESTING` define passed. `uv run --reinstall-package fastgpx
  pytest`: 207 passed.
- MSVC 19.51, configured per `Development.md` with `FASTGPX_BUILD_FUZZERS=ON`: `BUILD_TESTING: ON`,
  and Catch2 plus the corpus replays through CTest pass, 62/62.
- Sphinx with `-W --keep-going --fresh-env` builds clean.

Raw logs are outside the repository, in `~/fastgpx-review/v7/` (`build-wheel.log`,
`build-noinherit.log`, `build-on.log`, `plain.log`, `skb-on.log`, `skb-def.log`).

**Why.** Unhashable `TimeBounds` and `Bounds` cannot be lost in a set or dict after a change, and
they now follow the same rule as `LatLong` and `LatLongList`. Deciding the test default in CMake
removes the trap rather than documenting it: no pyproject override, with or without `inherit`,
can turn the tests on by accident, while asking for them explicitly still works.

**Not done.**

- The sanitized Clang build and a fuzz run were not repeated. Neither change touches the parsers,
  and the fuzz build sets no `SKBUILD`, which the plain configure above covers.
- No wheel was built with cibuildwheel. It drives scikit-build-core the same way, so the default
  should hold there, but that is inferred.
- `BUILD_TESTING` now sticks in a reused build directory. scikit-build-core keeps its build
  directory (`build/{wheel_tag}`), and the default applies only when the cache has no
  `BUILD_TESTING` yet. After one build with `-C cmake.define.BUILD_TESTING=ON`, later builds in
  that directory keep the tests until it is deleted. The old `BUILD_TESTING = false` define reset
  it on every configure. CI builds start from scratch and are not affected.
