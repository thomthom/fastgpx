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
  link-time optimization commit had the same exposure; not addressed.
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
