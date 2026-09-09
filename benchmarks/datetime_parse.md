# ISO 8601 parser comparison (`parse_gpx_time`)

`datetime.cpp` once held six parsers for the GPX `<time>` element, written to compare approaches
against each other. Only `v6`, `parse_gpx_time`, reached the production path; `v1` to `v5` were
removed in #47. This note keeps the comparison findable. The removed code is in the history before
that change (`git show a9ac49d:src/cpp/fastgpx/datetime.cpp`).

## The variants

| Variant | Approach                                                                        | Formats handled                                                |
|---------|---------------------------------------------------------------------------------|----------------------------------------------------------------|
| v1      | `std::istringstream` and `std::get_time`, retrying three format strings         | extended, basic and ordinal dates; fractional seconds          |
| v2      | `std::chrono::parse` into `utc_clock`                                           | `YYYY-MM-DDThh:mm:ssZ` only                                    |
| v3      | `std::chrono::parse` into `sys_time<milliseconds>`, retrying seven formats      | extended, basic, ordinal and week dates; fractions; UTC offsets |
| v4      | `std::from_chars` at fixed offsets, no validation                               | `YYYY-MM-DDThh:mm:ssZ` only                                    |
| v5      | Tokeniser over `std::views::chunk_by` covering most of ISO 8601                 | calendar dates, reduced precision, fractions, UTC offsets      |
| v6      | `std::from_chars` with range checks, dispatching on the string length           | the six GPX shapes listed in `datetime.hpp`                    |

`v2` and `v3` were the only users of `std::chrono::parse` and `std::chrono::utc_clock`. `v2` also
showed that `utc_clock::time_since_epoch()` counts leap seconds (27 s ahead of Unix time in 2024),
which is why the library stays on `system_clock`.

## Results

Catch2 `[!benchmark][datetime]`, parsing `"2024-05-18T06:50:01Z"`. Mean time per call over 100
samples, from three separate runs of the executable.

Setup: commit `a9ac49d`, MSVC 19.44 x64 RelWithDebInfo, Catch2 v3.7.1. The x64 binary ran under
emulation on an ARM64 machine (Snapdragon X X1P64100, Windows 11), so the absolute numbers are
higher than on native x64 hardware. An earlier native x64 measurement quoted in #47 (`v6` around
86 ns, the others 450 to 720 ns) was never written down beyond the issue. The ordering is the same.

| Variant                                | Run 1   | Run 2   | Run 3   |
|----------------------------------------|---------|---------|---------|
| v1 `std::get_time`                     | 1.55 us | 1.55 us | 1.58 us |
| v2 `std::chrono::parse` (`utc_clock`)  | 3.38 us | 3.56 us | 3.77 us |
| v3 `std::chrono::parse` (`sys_time`)   | 4.02 us | 4.02 us | 3.75 us |
| v4 `std::from_chars`, unchecked        | 70.0 ns | 74.9 ns | 67.3 ns |
| v5 tokenising parser                   | 2.60 us | 3.51 us | 3.06 us |
| v6 `parse_gpx_time`                    | 355 ns  | 324 ns  | 353 ns  |

Takeaways:

- Stream-based parsing (`v1`, `v2`, `v3`) is 4 to 12 times slower than `v6`. The
  `std::chrono::parse` variants are the slowest, and retrying format strings compounds the cost.
- `v4` is the fastest but validates nothing and accepts a single shape, so it was never a
  candidate for production.
- `v5` handles the most of ISO 8601, but the `unordered_map` lookup, chunk views and token vector
  make it as slow as the stream parsers.
- `v6` is within a factor of five of the unchecked lower bound while validating every field and
  covering the six shapes seen in real GPX files.

After removing `v1` to `v5` (same setup), `parse_gpx_time` alone:

| Run 1  | Run 2  | Run 3  |
|--------|--------|--------|
| 309 ns | 342 ns | 308 ns |

Unchanged within the run-to-run noise, as expected from deleting code that was never called.

## Raw output

Run 1 before the removal (commit `a9ac49d`):

```sh
Filters: [!benchmark] [datetime]
Randomness seeded to: 3937771536

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
fastgpx_test.exe is a Catch2 v3.7.1 host application.
Run with -? for options

-------------------------------------------------------------------------------
Benchmark parse iso8601 date string
-------------------------------------------------------------------------------
C:\Users\thoma\Source\fastgpx\src\cpp\fastgpx\datetime_test.cpp(676)
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
v1 std::get_time                               100            38     6.0648 ms 
                                        1.54876 us    1.54313 us    1.56942 us 
                                        49.0984 ns    12.6062 ns    113.082 ns 
                                                                               
v2 std::chrono::parse                          100            17     6.1523 ms 
                                        3.38347 us    3.35647 us    3.49941 us 
                                        244.543 ns    41.0502 ns    574.038 ns 
                                                                               
v3 std::chrono::parse                          100            16     6.2224 ms 
                                        4.02444 us    3.81219 us    4.43331 us 
                                        1.46098 us    839.206 ns    2.20641 us 
                                                                               
v4 std::from_chars                             100           831     5.9001 ms 
                                        70.0108 ns    69.4537 ns    71.3418 ns 
                                        4.11097 ns    1.94727 ns    7.90281 ns 
                                                                               
v5 std::from_chars parser                      100            24     6.1056 ms 
                                         2.6005 us    2.47721 us    3.05958 us 
                                        1.06749 us    265.242 ns    2.45414 us 
                                                                               
v6 std::from_chars gpx_time                    100           173     5.9685 ms 
                                        354.613 ns     338.41 ns    387.064 ns 
                                        112.538 ns      66.18 ns    174.781 ns 
                                                                               

===============================================================================
test cases: 1 | 1 passed
assertions: - none -

```

Run 1 after the removal:

```sh
Filters: [!benchmark] [datetime]
Randomness seeded to: 36119637

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
fastgpx_test.exe is a Catch2 v3.7.1 host application.
Run with -? for options

-------------------------------------------------------------------------------
Benchmark parse iso8601 date string
-------------------------------------------------------------------------------
C:\Users\thoma\Source\fastgpx\src\cpp\fastgpx\datetime_test.cpp(305)
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
parse_gpx_time                                 100           168     6.6192 ms 
                                        309.393 ns    308.851 ns    310.286 ns 
                                        3.46882 ns    2.44399 ns    5.34544 ns 
                                                                               

===============================================================================
test cases: 1 | 1 passed
assertions: - none -

```
