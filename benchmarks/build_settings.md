# Build settings

How the release wheels are built today, and the measurements that decided it. The experiments
that led there are kept below the line, under "Historical experiments". The full tables and build
hashes are in [review_decisions.md](review_decisions.md).

## Current configuration

| Setting | Where | Platforms |
|---|---|---|
| Release build type | `cmake.build-type` in `pyproject.toml` | all |
| Link-time optimization, only where the compiler supports it | `FASTGPX_LTO=ON` in the Linux override of `pyproject.toml` | Linux |
| Hidden visibility for pugixml and the core library | `src/cpp/CMakeLists.txt` | all; no effect with MSVC |
| nanobind's `NOMINSIZE` for the binding code | `FASTGPX_NOMINSIZE` in `src/cpp/CMakeLists.txt`, on by default except with MSVC | all but MSVC |
| Tests left out of package builds | `CMakeLists.txt` defaults `BUILD_TESTING` to off when `SKBUILD` is set | all |
| No 32-bit wheels | `CIBW_SKIP` in `.github/workflows/wheels.yml` | Windows `win32`, Linux `i686` |

Windows wheels are Release with neither link-time optimization nor `NOMINSIZE`.

## Evidence for the current configuration

### Linux: hidden visibility, `NOMINSIZE` and link-time optimization (`7a03e1d`)

Link-time optimization gains little in the extension without the other two settings. GCC's
inlining report at `b01d107` gave two reasons: pugixml's functions could be replaced at load time,
so GCC would not inline them into the parse loop, and nanobind's `-Os` copies of shared helpers
could not be inlined into the `-O3` parser. Hidden visibility makes the two static libraries
non-interposable, and `NOMINSIZE` compiles the binding code at `-O3`. The report was not re-read
for the shipped build, so how `NOMINSIZE` helps there is inferred.

WSL2 on the desktop, GCC 14.2, `gpx/sleipnir`, Sleipnir's upload path after
thomthom/sleipnir#596 (`no_copy`), ns per point, median of 5 alternated runs:

| Measurement | Release | hidden + `NOMINSIZE` + LTO (shipped) | hidden + LTO | hidden + `NOMINSIZE` |
|---|---:|---:|---:|---:|
| `fastgpx.parse(text)` | 167.4 | 136.6 | 157.3 | 154.3 |
| Upload path total | 359.4 | 322.4 | 348.4 | 341.0 |
| Upload path total, `gpx/TET` | 320.5 | 301.6 | 307.9 | 317.3 |

Parsing is 18% faster and the upload path 10% faster than a plain Release build. A reviewer's build
in the `manylinux_2_28` image, which production's wheels come from, showed the same gain (parse
175.1 → 144.8, total 392.1 → 347.9).

The shipped Linux extension is also smaller, because the hidden symbols are no longer exported:
377,104 bytes against 572,504, and a 173,184-byte wheel against 233,731, with Ubuntu's GCC 14.
From the `manylinux_2_28` image, which links parts of libstdc++ statically, the extension is
899 KB and the wheel 410 KB. The CI wheel build of `6b5aac1` produced a 408 KB x86_64 wheel.

### Linux: link-time optimization alone made Python parsing slower (`b01d107`)

Before the visibility change, link-time optimization made parsing from Python slower on the same
files, not faster: 186 against 169 ns per point. This is why it was briefly dropped, and why it
now comes only with hidden visibility and `NOMINSIZE`.

### Linux: the known cost, `parse_gpx_time`

With the shipped settings, parsing a single timestamp in C++ takes 35.0 ns against 23.6 ns in a
plain Release build (`7a03e1d`, Catch2, median of 5 runs). It does not reach Sleipnir's upload
path, which never reads a point's time. Time bounds, the bulk timestamp work on that path, got
faster: 11.7 → 9.8 ns per point.

Why this happens was worked out on the earlier link-time optimization build; see "Why link-time
optimization slows `parse_gpx_time` on GCC" below. That the same mechanism applies to the shipped
build is inferred, not re-checked.

### Link-time optimization only where supported (`9a39c91`)

`FASTGPX_LTO` asks for link-time optimization, and CMake's `check_ipo_supported` decides. Where
it is not supported, configure warns and the build goes on without it. Before, the Linux override
forced it, and a build from the sdist with a toolchain that cannot do it failed. A Linux wheel
build with GCC 14.2 issues the same 19 build commands before and after the change, so the
benchmarks were not re-run. The fallback was checked with a Clang 18 whose archiver was made
unreachable, which is a simulation.

### Windows: Release only (`7a03e1d`)

MSVC 19.51, ns per point. The upload-path rows are `gpx/sleipnir`, median of 5 alternated runs.
The polyline rows cover every segment of `gpx/sleipnir`, best of 7, over three alternated runs:

| Measurement | Release (shipped) | `NOMINSIZE` | LTO | `NOMINSIZE` + LTO |
|---|---:|---:|---:|---:|
| `gpx/sleipnir` `fastgpx.parse(text)` | 394.6 | 406.4 | 334.8 | 325.9 |
| `gpx/sleipnir`, `no_copy` total | 652.0 | 659.8 | 592.2 | 580.4 |
| `polyline.encode` from Python | 10.1 | 10.2 | 10.9 | 11.1 |
| `polyline.decode` from Python | 60 | 59 | 63 | 64 |

`NOMINSIZE` alone made parsing slightly slower. Link-time optimization makes the upload path about
11% faster, but polyline work slower: 4–33% in C++ and 5–8% from Python. Windows is used for
development, not production, so it stays as it was. This is worth revisiting if the Windows
upload path ever matters.

### Tests left out of package builds (`e3f3651`)

When scikit-build-core drives the build, `CMakeLists.txt` defaults `BUILD_TESTING` to off. Before,
`pyproject.toml` set it, and a platform override could replace that table and turn the tests back
on, which happened once in the Linux wheels. Checked in WSL2 with GCC 14.2 and CMake 4.4.3:

- A wheel build configures with `BUILD_TESTING` off and does not fetch Catch2.
- `-C cmake.define.BUILD_TESTING=ON` still turns the tests on.
- A plain CMake configure still has them on.

### No 32-bit wheels (`efdcdf3`)

`nanobind-backend`, which the split-mode extension depends on, publishes no wheels for 32-bit
Windows or Linux, so those wheel builds failed. They are skipped now. Releases 0.5.0 to 0.7.0
shipped a `win32` wheel; the next release will not. The CI wheel build of `6b5aac1` built the
sdist and all four remaining wheels.

### Release rather than RelWithDebInfo (`4c0b6be`)

The wheels used to be RelWithDebInfo. On MSVC that restricts inlining, and on Linux it ships the
extension with about 12 MB of debug info. Release was faster or the same in every measurement at
`6d4e491`, for example 1.67× for `polyline::decode` and 1.13× for time bounds in C++ on Linux.
Those measurements are under "Historical experiments", because they were made on the build of the
time, and their C++ figures were not re-measured since.

---

## Historical experiments

These experiments came before the current configuration. They are kept as they were written,
including decisions that were later reversed. Each block says why it is here. Their C++ figures
were not re-measured, and some rows are over an unrecorded collection of files, so prefer the
sections above where both cover the same thing.

### Release, link-time optimization and `NOMINSIZE` at `6d4e491`

Historical because it was measured on the code and build of the time, before hidden visibility
existed. Its Windows `polyline::decode` figure (139 µs) was not reproduced later: `7a03e1d`
measured 250–330 µs, with no explanation found. The "183 real files" rows are over an unrecorded
collection.

When this was written, the release wheels were built as RelWithDebInfo (`cmake.build-type` in
`pyproject.toml`). This compares that with Release and with link-time optimization, on the same
code (commit `6d4e491`), to decide what the wheels should use. See [load_profile.md](load_profile.md)
for why it came up.

The "183 real files" below were not recorded. They were most likely all 145 files of Sleipnir's
upload folder, duplicates included, plus the 34 TET routes and the 4 in Sleipnir's `gpx/tet`;
that is unconfirmed. New measurements use the folders in [corpus_manifest.json](corpus_manifest.json).

#### Variants

| | Build | MSVC 19.51 | GCC 14.2 |
|---|---|---|---|
| A | RelWithDebInfo, as shipped | `/O2 /Ob1`, linked `/INCREMENTAL` | `-O2 -g`, not stripped |
| B | Release | `/O2 /Ob2` | `-O3`, stripped |
| C | Release + link-time optimization | B + `/GL /LTCG` | B + `-flto` |
| D | Release, binding code not optimized for size | B, extension without `/Os` | – |

nanobind compiles the extension module (not the parser) with `/Os` / `-Os` in every optimized build
type, and only strips it and drops the stack protector in Release. D turns the size optimization
off with nanobind's `NOMINSIZE` option. Linux was measured in C++ only.

Same machine as the other notes (Ryzen 7 5800X; Linux is WSL2). Median of five runs, variants run
in alternation. Speedup is against A; below 1× is slower.

#### Windows

| Measurement | A | B | C | D |
|---|---:|---:|---:|---:|
| `LoadGpx`, TET files (C++) | 482 ns/point | 1.06× | 1.16× | – |
| `load()`, 183 real files (Python) | 1.50 s | 1.04× | 1.18× | 1.03× |
| `time_bounds()`, 183 real files (Python) | 39.7 ms | 1.28× | 1.37× | 1.29× |
| `length_2d()`, 183 real files (Python) | 71.4 ms | 0.99× | 1.04× | 0.99× |
| Reading every point's coordinates (Python) | 22.1 ms | 1.13× | 1.13× | 1.12× |
| `polyline.encode`, all segments of one file (Python) | 2.93 ms | 1.12× | **0.43×** | 1.13× |
| `polyline::encode`, 10k points (C++) | 187 µs | 1.16× | **0.38×** | – |
| `polyline::decode`, 10k points (C++) | 139 µs | 1.16× | 0.87× | – |
| `parse_gpx_time` (C++) | 70 ns | 1.05× | 1.02× | – |

#### Linux

| Measurement | A | B | C |
|---|---:|---:|---:|
| `LoadGpx`, TET files (C++) | 138 ns/point | 1.01× | 1.23× |
| Parse a 20k-point file (C++) | 4.10 ms | 1.08× | 1.23× |
| Time bounds of a 5,189-point segment (C++) | 70 µs | 1.13× | 1.32× |
| `polyline::encode`, 10k points (C++) | 127 µs | 1.00× | 0.96× |
| `polyline::decode`, 10k points (C++) | 101 µs | 1.67× | 1.74× |
| `parse_gpx_time` (C++) | 37 ns | 1.56× | 1.03× |

#### Size

| | A | B |
|---|---:|---:|
| Windows `fastgpx.pyd` | 1.47 MB | 0.45 MB |
| Linux `fastgpx.abi3.so` in the published 0.7.0 wheel | 13.6 MB, of which 12 MB is debug info | stripped |

The Linux wheel is about ten times the size of the Windows one because RelWithDebInfo is never
stripped.

#### Findings

- **Release is faster than what ships, or the same, in every measurement.** Nothing got slower.
  The gains are modest for loading (6% on Windows, none on Linux), but larger elsewhere: time
  bounds 13–35%, polyline decoding 16–67%, the binding layer about 12%. It also removes 12 MB of
  debug info from the Linux wheel.
- **Link-time optimization speeds up loading by 16–23%** on both platforms, because it lets the
  compiler inline pugixml's lookups into fastgpx's loop. But with MSVC it makes polyline encoding
  2.6 times slower, consistently in every round. With GCC, polyline encoding is unaffected. It also
  undoes Release's gain on `parse_gpx_time` with GCC, though bulk timestamp work (time bounds) is
  still fastest with it.
- **Optimizing the binding code for speed instead of size (D) gains nothing measurable.** nanobind's
  default can stay.

### Why link-time optimization slows polyline encoding on MSVC

Historical because the encoder no longer builds a temporary string per value (`e66b8a2`), so the
code this explains is gone.

Profiled with a loop that only calls `polyline::encode` on the benchmark's 10k points: 171 µs per
call with B, 368 µs with C. With C, 70% of the samples land on five instructions, and each follows
the same pattern.

`encode` builds a temporary `std::string` for every value and appends one character at a time.
A string that short lives in `std::string`'s built-in buffer, which shares its space with the
pointer used for longer strings. With link-time optimization, the helper that encodes one value
is inlined into `encode`, and MSVC then picks between the built-in buffer and the pointer without
a branch. So after every character it writes into the buffer, it reads the pointer field, which
at that moment holds the characters just written. The processor cannot forward those one-byte
writes to an eight-byte read, so the read waits for the writes to finish: a store-forwarding stall,
once per encoded character. Without link-time optimization the same read sits behind a branch
that skips it for short strings.

So the regression comes from how the encoder builds its output, not from link-time optimization
as such. The encoder now writes the characters straight into the result string, without a
temporary per value. After that change, encoding 10k points on Windows takes 74 µs in Release and
81 µs with link-time optimization, down from 165 µs and 484 µs (see
[performance.md](performance.md)).

### Link-time optimization after the encoder change

Historical because it measured the earlier link-time optimization build, before hidden visibility
and `NOMINSIZE`. Its decision was reversed and then restored in a different form; the current
configuration is at the top of this file.

Measured again at `e66b8a2`, Release (B) against Release + link-time optimization (C). Median of
five runs, alternated; Windows had background load, so smaller Windows differences are uncertain.

| Measurement | Windows C/B | Linux C/B |
|---|---:|---:|
| `LoadGpx`, TET files (C++) | 1.16× | 1.22× |
| `load()`, 183 real files (Python) | 1.08× | – |
| Time bounds of a 5,189-point segment (C++) | within noise | 1.12× |
| `polyline::encode`, 10k points (C++) | within noise | within noise |
| `polyline::decode`, 10k points (C++) | **0.73×** | within noise |
| `parse_gpx_time` (C++) | **0.93×** | **0.69×** |
| `polyline.encode`, one file (Python) | 0.94× | – |

Above 1× is faster with link-time optimization.

**Decision:** link-time optimization for the Linux wheels only, through a platform override in
`pyproject.toml`. Linux is where production runs, and there it speeds up loading and time bounds
with no polyline cost. The `parse_gpx_time` slowdown does not show in bulk timestamp work, which
is faster. On Windows, which is used for development, polyline decoding would be about a quarter
slower.

**Reversed** after measuring Sleipnir's upload path from Python, where link-time optimization made
parsing slower on Linux, not faster, with the extension built as it is (nanobind's `-Os` and
interposable pugixml symbols block the inlining). The wheels are now Release everywhere; see
[review_decisions.md](review_decisions.md).

**Restored for Linux** once those two blocks were removed: pugixml and the core library are now
built with hidden visibility, and the Linux extension with nanobind's `NOMINSIZE`. With link-time
optimization, Sleipnir's upload path is 10% faster than the Release build and parsing 18% faster.
Windows is unchanged. See the item "build the Linux wheels so that link-time optimization reaches
the parser" in [review_decisions.md](review_decisions.md).

A Linux wheel built with the restored override: a 377 KB extension and a 173 KB wheel with
Ubuntu's GCC 14, and 899 KB and 410 KB from the `manylinux_2_28` image, which links parts of
libstdc++ statically (the published 0.7.0 wheel is 3.6 MB).

Not yet explained: why link-time optimization slows polyline decoding on MSVC.

### Why link-time optimization slows `parse_gpx_time` on GCC

Historical because it was worked out on the earlier link-time optimization build. The slowdown
itself is still there with the current settings (23.6 against 35.0 ns, `7a03e1d`); whether the
cause is the same was not re-checked.

Timestamps are read field by field through a small helper, `StringParser::ExtractInt`, which
takes the number of digits as an argument. In a plain Release build, GCC inlines five of its six
calls in the date and time part, so each copy sees a fixed digit count. With link-time
optimization, none of them are inlined. GCC's own report gives the reason: its limit on how much
a unit may grow through inlining (`inline-unit-growth`). A single source file stays under the
threshold where that limit applies, but the whole program does not.

In a tight loop that only parses timestamps, this costs about 11 ns per call, 25 against 36 ns.
Other sessions measured 24 against 36 ns (the table below), 24 against 35 ns
([performance.md](performance.md)) and 23.6 against 35.0 ns with the current Linux wheel settings
([review_decisions.md](review_decisions.md)); the differences are between sessions.
It does not show in real use. Time bounds, which parse only the first and last timestamp of a
segment, are faster with link-time optimization, and reading every point's time costs the same
(about 49 against 50 ns per point, one round each). Loading does not parse timestamps at all.

Two fixes were tried and reverted, both on GCC 14 in WSL2, median of five alternated runs:

| Attempt | Release | Release + link-time optimization |
|---|---:|---:|
| Before either | 24 ns | 36 ns |
| Digit count as a template parameter, `ExtractInt<N>()` | 28 ns | 31 ns |
| Force-inlining `ExtractInt` | 38 ns | 45 ns |

The template kept the calls out of line and made the Release build slower (MSVC too, by 20%).
Force-inlining did inline every call, but made both builds slower. So inlining alone was not what
made the Release build fast: even there, one call stays out of line. The machine code of the fast
build was not examined; that would be the next step if this path ever matters.
