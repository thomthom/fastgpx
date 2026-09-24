# Where Sleipnir's GPX upload spends fastgpx time

Sleipnir turns an uploaded GPX file into stored tracks and segments in `create_gpx_file`
(`apps/maps/gpx_ingest.py`). This measures the fastgpx side of that path, step by step, to see
whether parsing or the Python work around it is worth improving. The Django `LineString`
construction and the database writes are not included.

## Setup

- Files: 140 unique GPX files, 3 million track points: the 34 TET country routes and the uploads
  on the Sleipnir dev server. Most upload points carry a timestamp; the TET points mostly do not.
- Linux: WSL2 on the desktop from [performance.md](performance.md), the Release + link-time
  optimization wheel as shipped, Python 3.12. Windows: the same machine, the Release wheel.
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

`ingest_bench.py` mirrors the fastgpx calls in `create_gpx_file` and times each step. It is kept
outside the repository with the other benchmark scripts:

```sh
python ingest_bench.py <gpx folder> [<gpx folder> ...]
```

On WSL, read the files from WSL's own file system, not `/mnt/c`, or `load()` measures the file
share rather than fastgpx.
