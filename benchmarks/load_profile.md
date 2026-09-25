# Where `LoadGpx` spends its time

> **Status:** historical. Profiled 2026-09-24 at `1b8881a` (committed in `6d4e491`), RelWithDebInfo
> with MSVC 19.51 and GCC 14.2. Not re-profiled since the wheels became Release (`4c0b6be`), the
> trim it flagged was rewritten (`441ba44`), and Linux gained link-time optimization that reaches
> the parser (`7a03e1d`).

A CPU profile of loading GPX files, made to find what is worth improving next. It was taken on
Windows and on Linux (WSL) on the same machine, because the answer turned out to depend on the
platform. It records the starting point: the builds profiled here are RelWithDebInfo, as the
wheels were then. Since then the wheels became Release (with link-time optimization on Linux) and
the whitespace trim was rewritten; see [performance.md](performance.md).

## Setup

|          | Windows                                                | Linux                           |
|----------|--------------------------------------------------------|---------------------------------|
| Machine  | AMD Ryzen 7 5800X, 32 GB, Windows 11 Pro 25H2          | Same machine, WSL2 Ubuntu 24.04 |
| Compiler | MSVC 19.51                                             | GCC 14.2                        |
| Build    | RelWithDebInfo: `/O2 /Ob1`, linked `/INCREMENTAL`      | RelWithDebInfo: `-O2 -g`        |
| Profiler | Windows Performance Recorder, CPU sampling with stacks | `perf record`, user code only   |

Both builds use the build type the release wheels used at the time (`cmake.build-type` in
`pyproject.toml`).
The code is commit `1b8881a`. The workload is `profile_load` (`src/cpp/profile_load.cpp`, built
along with the tests), which calls `LoadGpx` on the 34 TET country routes (135 MB, 1.66 million
track points), 20 passes: `profile_load gpx/TET 20`. It measures `LoadGpx` alone, without the
Python bindings.

## Result

Linux loads the same files 3.3 times faster: **140 ns per point against 461 ns** on Windows.

Time per track point, by area:

| Area                                                      |    Windows |        Linux |
|-----------------------------------------------------------|-----------:|-------------:|
| Converting numbers (`std::from_chars`)                    |     212 ns |        34 ns |
| Rest of `TryParseDouble`, mostly trimming whitespace      |      57 ns |        18 ns |
| pugixml building the XML tree                             |      45 ns |        47 ns |
| Looking up attributes and elements by name                |      50 ns |        21 ns |
| fastgpx's own loop and point lists                        |      40 ns |        15 ns |
| Kernel and memory: reading the file, freeing, page faults |     ~90 ns | not measured |
| **Total**                                                 | **461 ns** |   **140 ns** |

The shares come from the profiles and are converted to nanoseconds with the total from a run
without the profiler. The Linux profile could not see kernel time, so its rows are slightly high.

## Findings

- **Number conversion is the platform difference.** MSVC's `std::from_chars` takes about 70 ns
  per number on these coordinates, which have 15 to 17 digits. GCC's takes about 11 ns. On Windows
  that is nearly half of all load time.
- **Building the XML tree costs the same on both,** about 46 ns per point. pugixml is compiled from
  source into fastgpx, so this part does not depend on the platform's standard library. On Linux it
  is the largest area, about a third of load time.
- **The whitespace trim is expensive for what it does.** `TryParseDouble` trims every number with
  `find_first_not_of` and `find_last_not_of`, although GPX numbers almost never have whitespace.
  It is 13% of load time on Linux and 10% on Windows. On Windows, most of that 10% is MSVC's
  vectorised search choosing a strategy for strings that are only about 17 characters long.
- **Name lookups add up.** pugixml finds attributes and child elements by comparing names
  (`strcmp`), and attribute values have no stored length, so each one is measured with `strlen`.
  Together they are about 15% on Linux and 11% on Windows.
- **On Windows, a fifth of the time is in the kernel and the C runtime.** Reading the file is
  about 9%. Freeing memory is about 4%, mostly the file buffer, which is large enough to be
  returned to Windows after every load. Page faults are about 3%, and the point list growing
  without a reserve is about 3%.
- **Windows Defender costs nothing.** Its filter driver sits in the path of every file read, even
  in excluded folders, but its own time was 2 ms over the whole run. This profile ran in an
  excluded folder.
- **Earlier conclusion revised.** #23 found that pugixml's tree build was essentially all of the
  load time (GCC, Linux). That still roughly holds on Linux, but not on Windows, where fastgpx's
  own pass over the tree is 80% of load time.

## Build settings of the release wheels

At the time, the wheels were built as RelWithDebInfo. On MSVC that means `/Ob1`, so only functions
marked `inline` are inlined, and the extension is linked `/INCREMENTAL`. The profile shows the effect:
`ParseCoordinate`, `TryParseDouble` and the `LatLong` constructor are real calls for every point,
and incremental-link thunks take about 1%. On GCC, RelWithDebInfo is `-O2` against Release's
`-O3`. [build_settings.md](build_settings.md) has since measured Release, and the wheels are now
built that way.

## Where to look next

In order of what they would save on Linux, where production runs:

1. The pugixml tree build, about a third.
2. `std::from_chars`, about a quarter. On Windows, by far the largest cost.
3. The whitespace trim in `TryParseDouble`, 13%. Since rewritten.
4. Name lookups (`strcmp` and `strlen`), about 15%.
5. The build type of the release wheels. Since changed to Release.

## Reproducing

- Windows: record with `wpr -start CPU` / `wpr -stop trace.etl` around the workload, and read it
  with `xperf -i trace.etl -symbols -a profile -detail` or `-a stack -butterfly`. Point
  `_NT_SYMBOL_PATH` at the build folder only. With Microsoft's symbol server on the path, `xperf`
  downloads symbols for every module of every process in the trace, which runs to gigabytes.
- Linux: `perf record -F 999 -e cpu-clock:u --call-graph dwarf` around the workload, then
  `perf report --no-children --sort symbol`. Build and read the files on WSL's own file system,
  not under `/mnt/c`.
