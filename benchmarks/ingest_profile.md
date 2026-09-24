# Where Sleipnir's GPX upload spends fastgpx time

> **Status:** historical. Measured 2026-09-24 for `f36982d`, with the earlier Linux link-time
> optimization wheel that did not reach the parser. Its two findings became thomthom/sleipnir#596
> and `Segment.lonlat()` (`b841e82`). Current upload-path numbers are in
> [performance.md](performance.md#where-things-stand).

Sleipnir turns an uploaded GPX file into stored tracks and segments in `create_gpx_file`
(`apps/maps/gpx_ingest.py`). This measures the fastgpx side of that path, step by step, to see
whether parsing or the Python work around it is worth improving. The Django `LineString`
construction and the database writes are not included.

## Setup

- Files: 140 unique GPX files, 3 million track points: the 34 TET country routes and the uploads
  on the Sleipnir dev server. Most upload points carry a timestamp; the TET points mostly do not.
- Linux: WSL2 on the desktop from [performance.md](performance.md), the Release + link-time
  optimization wheel as shipped at `f36982d`, Python 3.12. That is the earlier link-time
  optimization build that did not reach the parser; see [review_decisions.md](review_decisions.md)
  for current numbers. Windows: the same machine, the Release wheel.
- Best of five per file, every run parsing afresh (bounds, lengths and time bounds are cached on
  the parsed objects).

## Result

Time per track point:

| Step | Linux | Windows |
|---|---:|---:|
| `content.decode("utf-8")` | 15 ns | 23 ns |
| `fastgpx.parse(text)` | 177 ns | 424 ns |
| `list(segment.points)` | 77 ns | 98 ns |
| A new `LatLong` per point, copying latitude and longitude | 145 ns | 174 ns |
| `(lon, lat)` tuples for GEOS | 97 ns | 106 ns |
| `length_2d()`, bounds, time bounds | 39 ns | 37 ns |
| **Total** | **550 ns** | **862 ns** |

On Linux, parsing is about a third of the fastgpx time. The three passes over the points after
parsing are more than half.

Parsing from Python compared with C++, same files, Linux:

| | ns/point |
|---|---:|
| C++ `LoadGpx` | 142 |
| Python `fastgpx.load(path)` | 166 |
| Python `fastgpx.parse(text)` | 177 |
| Python `content.decode()` + `fastgpx.parse(text)` | 193 |

## Findings

- **The copy is not needed.** `latlong_list_to_linestring` only reads latitude and longitude, so
  the original points give the same `LineString`. Dropping it saves about a quarter of the
  fastgpx time of an upload. Filed as thomthom/sleipnir#596.
- **The rest of the per-point cost is the binding layer.** A Python object per point and an
  attribute lookup per coordinate, to end up with a list of tuples. A bulk accessor that builds
  the coordinates in C++ would remove most of it. Proposed in #73.
- **Getting the parsed file into Python is cheap.** Python adds about 50 ns per point to C++
  parsing: 16 for decoding the bytes, 11 for parsing from a `str` rather than a file, 24 for
  going through Python at all. The per-point work after parsing costs several times that.
- **Python 3.14 is no faster than 3.12** here. On Linux the totals were equal, 1.65 against
  1.66 s; on Windows 3.14 was slightly slower. One run each.

## Reproducing

The script used for the numbers above was kept outside the repository and is lost.
[benchmark_ingest.py](benchmark_ingest.py) replaces it: it repeats the fastgpx calls of the upload
view and `create_gpx_file` in the same order and times each step per point. It also times the
path after thomthom/sleipnir#596, without the `LatLong` copy, and records which build, Python and
machine produced a run. Its steps are split a little finer than the table above, so compare new
numbers with each other rather than with this table.

```sh
uv run benchmarks/benchmark_ingest.py run gpx/sleipnir gpx/TET -o before.json
uv run benchmarks/benchmark_ingest.py compare before.json after.json
```

The 140 files are most likely `gpx/sleipnir` and `gpx/TET` as listed in
[corpus_manifest.json](corpus_manifest.json): together they are 140 unique files and 3.0 million
track points. `uv run benchmarks/corpus_manifest.py verify` checks a local copy against it.

On WSL, read the files from WSL's own file system, not `/mnt/c`, or `load()` measures the file
share rather than fastgpx.
