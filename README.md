# fastgpx

An experimental Python library for parsing GPX files fast.

```py
# Get the total length of the tracks in a GPX file:
import fastgpx

gpx = fastgpx.load("example.gpx")
print(f'{gpx.length_2d()} m')
```

```py
# Iterate over GPX file:
import fastgpx

gpx = fastgpx.load("example.gpx")
for track in gpx.tracks:
    print(f'Track: {track.name}')
    print(f'Distance: {track.length_2d()} m')
    time_bounds = track.time_bounds()
    if not time_bounds.is_empty():
        print(f'Time: {time_bounds.start_time} - {time_bounds.end_time}')
    for segment in track.segments:
        for point in segment.points:
            print(f'Point: {point.latitude}, {point.longitude}')
```

```py
import fastgpx

locations = [
    fastgpx.LatLong(64, 10),
    fastgpx.LatLong(66, 11),
]
encoded = fastgpx.polyline.encode(locations, precision=6)

decoded = fastgpx.polyline.decode(encoded, precision=6)
```

[Documentation](https://thomthom.github.io/fastgpx/)

## Requirements

* Python 3.12+ (Tested with 3.12, 3.13, 3.14)
* C++23 Compiler (For building `fastgpx`)

### Windows

* Tested with MSVC 17.12.4+ and Clang-cl 19+.

### Linux (Tested on Ubuntu)

* C++23 compatible runtime (GCC libstdc++ 14+ or Clang libc++ 18.1+)

## GPX/XML Performance (Background)

This library came out of the need to extract information from many GPX files fast.

`gpxpy` is the most popular GPX library for Python. It is very versatile in manipulating GPX files.

However in benchmarking it doesn't perform well.

`gpxpy` docs says (at time of writing) that it uses `lxml` is available because it is faster than
"`minidom`" (`etree`).

When benchmarking that was not the case. It appear that the stdlib XML library has gotten much
better since `gpxpy` was created.

Reference: Open ticket on making `etree` default:
https://github.com/tkrajina/gpxpy/issues/248

`fastgpx` is not intended as a replacement for `gpxpy`. It mainly focuses on extracting GPX data
fast for performance critical tasks. For the few functionalities that does overlap with `gpxpy`
compatible method calls has been added so that one can quickly swap between `fastgpx` and `gpxy`.

## Benchmarks

Test machine: AMD Ryzen 7 5800X, 32 GB memory, Windows 11, and WSL2 Ubuntu 24.04 on the same
machine. Python 3.12, fastgpx 0.8.0, gpxpy 1.6.2, lxml 6.1.3, polyline 2.0.4.

### Total track length

Total track length of `gpx/2024 Great Roadtrip` (24 files, 330k points), in seconds per pass.
Lower is better.

| Method                             | Windows |  Linux |
|------------------------------------|--------:|-------:|
| gpxpy                              |    13.3 |   11.7 |
| `xml.etree` + `gpxpy.geo` distance |   0.700 |  0.610 |
| lxml + `gpxpy.geo` distance        |   0.892 |  0.593 |
| fastgpx (`load` + `length_2d`)     |   0.159 | 0.0538 |

gpxpy's and fastgpx's lengths differ by 0.08%, because they use different distance formulas.

### Polyline encoding

Encoding every segment of the same files, in seconds per pass. Both timings include loading the
files with fastgpx.

| Method                    | Windows |  Linux |
|---------------------------|--------:|-------:|
| `fastgpx.polyline.encode` |   0.155 | 0.0464 |
| `polyline.encode`         |   0.655 |  0.411 |

### Reproducing

```sh
uv run --group benchmarks benchmarks/benchmark_gpx.py
uv run --group benchmarks benchmarks/benchmark_polyline.py
```

`benchmark_gpx.py` prints each method's time per pass as "Average"; gpxpy runs once.
`benchmark_polyline.py` prints the total for 10 passes, so divide it by 10 to compare with the
table.

Detailed performance notes are in [benchmarks/README.md](benchmarks/README.md).
