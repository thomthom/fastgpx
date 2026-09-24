# Build settings

The release wheels are built as RelWithDebInfo (`cmake.build-type` in `pyproject.toml`). This
compares that with Release and with link-time optimization, on the same code (commit `6d4e491`),
to decide what the wheels should use. See [load_profile.md](load_profile.md) for why it came up.

## Variants

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

## Windows

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

## Linux

| Measurement | A | B | C |
|---|---:|---:|---:|
| `LoadGpx`, TET files (C++) | 138 ns/point | 1.01× | 1.23× |
| Parse a 20k-point file (C++) | 4.10 ms | 1.08× | 1.23× |
| Time bounds of a 5,189-point segment (C++) | 70 µs | 1.13× | 1.32× |
| `polyline::encode`, 10k points (C++) | 127 µs | 1.00× | 0.96× |
| `polyline::decode`, 10k points (C++) | 101 µs | 1.67× | 1.74× |
| `parse_gpx_time` (C++) | 37 ns | 1.56× | 1.03× |

## Size

| | A | B |
|---|---:|---:|
| Windows `fastgpx.pyd` | 1.47 MB | 0.45 MB |
| Linux `fastgpx.abi3.so` in the published 0.7.0 wheel | 13.6 MB, of which 12 MB is debug info | stripped |

The Linux wheel is about ten times the size of the Windows one because RelWithDebInfo is never
stripped.

## Findings

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

## Why link-time optimization slows polyline encoding on MSVC

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

## Link-time optimization after the encoder change

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

A Linux wheel built with the override: Release with link-time optimization, stripped, a 493 KB
extension and a 208 KB wheel (the published 0.7.0 wheel is 3.6 MB).

Not yet explained: why link-time optimization slows polyline decoding on MSVC.

## Why link-time optimization slows `parse_gpx_time` on GCC

Timestamps are read field by field through a small helper, `StringParser::ExtractInt`, which
takes the number of digits as an argument. In a plain Release build, GCC inlines five of its six
calls in the date and time part, so each copy sees a fixed digit count. With link-time
optimization, none of them are inlined. GCC's own report gives the reason: its limit on how much
a unit may grow through inlining (`inline-unit-growth`). A single source file stays under the
threshold where that limit applies, but the whole program does not.

In a tight loop that only parses timestamps, this costs about 11 ns per call, 25 against 36 ns.
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
