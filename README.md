# fastgpx

An experimental Python library for parsing GPX files fast.

```py
# Get the total length of the tracks in a GPX file:
import fastgpx

gpx = fastgpx.parse("example.gpx")
print(f'{gpx.length_2d()} m')
```

```py
# Iterate over GPX file:
import fastgpx

gpx = fastgpx.parse("example.gpx")
for track in gpx.tracks:
    print(f'Track: {track.name}')
    print(f'Distance: {track.length_2d()} m')
    if not track.time_bounds.is_empty():
      print(f'Time: {track.time_bounds().start_time} - {track.time_bounds().end_time}')
    for segment in track.segments:
        print(f'Segment: {segment.name}')
        for point in segment.points:
            print(f'Point: {point.latitude}, {point.longitude}')
```

[Documentation](https://thomthom.github.io/fastgpx/)

## Requirements

* Python 3.11+ (Tested with 3.11, 3.12)
* C++23 Compiler

### Windows

* Tested with MSVC 17.12.4+ and Clang-cl 19+.

### Linux (Tested on Ubuntu)

* C++23 compatible runtime (GCC libstdc++ 14+ or Clang libc++ 18.1+)

## GPX/XML Performance (Background)

`gpxpy` appear to be the most popular GPX library for Python.

`gpxpy` docs says that it uses `lxml` is available because it is faster than "`minidom`" (`etree`).
When benchmarking that seemed not to be the case. It appear that the stdlib XML library has gotten
much better since `gpxpy` was created.

Reference: Open ticket on making `etree` default:
https://github.com/tkrajina/gpxpy/issues/248

## Benchmarks

Test machine:

* AMD Ryzen 7 5800 8-Core, 3.80 GHz
* 32 GB memory
* m2 SSD storage

### gpxpy benchmarks

Comparing getting the distance of a GPX file using `gpxpy` vs manually extracting
the data using `xml_etree`, computing distance between points using `gpxpy`
distance functions.

#### gpxpy without `lxml`

```
Running benchmark with 3 iterations...
gpxpy 5463041.784135511 meters
gpxpy 5463041.784135511 meters
gpxpy 5463041.784135511 meters
gpxpy: 11.497863 seconds (Average: 3.832621 seconds)
```

#### gpxpy with `lxml`

```
Running benchmark with 3 iterations...
gpxpy 5463041.784135511 meters
gpxpy 5463041.784135511 meters
gpxpy 5463041.784135511 meters
gpxpy: 37.803625 seconds (Average: 12.601208 seconds)
```

#### xml_etree data extraction

```
Running benchmark with 3 iterations...
xml_etree 5463043.740615641 meters
xml_etree 5463043.740615641 meters
xml_etree 5463043.740615641 meters
xml_etree: 2.333200 seconds (Average: 0.777733 seconds)
```

Even with `gpxpy` using `etree` to parse the XML it is paster to parse it
directly with `etree` and use `gpxpy.geo` distance functions to compute the
distance of a GPX file. Unclear what the extra overhead is, possibly the cost
of extraction additional data. (Some minor difference in how the total distance
is computed in this example. Using different options for computing the distance.)

### C++ benchmarks

Since XML parsing itself appear to have a significant impact on performance some
popular C++ XML libraries was tested:

#### tinyxml2
```
Total Length: 5456930.710560566
Elapsed time: 0.4980144 seconds
```

#### pugixml
```
Total Length: 5456930.710560566
Elapsed time: 0.1890089 seconds
```

### C++ vs Python implementations


```
Running 5 benchmarks with 3 iterations...

Running gpxpy ...
gpxpy: 50.182288 seconds (Average: 16.727429 seconds)

Running xml_etree ...
xml_etree: 8.269050 seconds (Average: 2.756350 seconds)

Running lxml ...
lxml: 8.479702 seconds (Average: 2.826567 seconds)

Running tinyxml (C++) ...
tinyxml (C++): 2.699880 seconds (Average: 0.899960 seconds)

Running pugixml (C++) ...
pugixml (C++): 0.381095 seconds (Average: 0.127032 seconds)
```

For computing the length of a GPX file, `pugixml` in a Python C extension was ~140
times faster than using `gpxpy`.
