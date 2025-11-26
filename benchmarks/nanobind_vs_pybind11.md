# nanobind vs pybind11

## Test machine

| Device name   |	HAL-9000                                      |
|---------------|-----------------------------------------------|
| Processor     | AMD Ryzen 7 5800X 8-Core Processor (3.80 GHz) |
| Installed RAM | 32.0 GB                                       |
| Device ID     | 8AA94896-2D6E-4059-ADD5-56D0F61D3C16          |
| Product ID    | 00330-80136-92201-AA878                       |
| System type   | 64-bit operating system, x64-based processor  |
| Pen and touch | Pen support                                   |
| OS            | Windows 11                                    |


## GPX parsing and distance calculation

### nanobind
```
> uv run benchmark_gpx.py

Computing expected distance with gpxpy...
gpxpy 5839236.226642554 meters
gpxpy: 15.318279 seconds

Running 3 benchmarks with 3 iterations...

xml_etree: 2.046208 seconds (Average: 0.682069 seconds)
lxml: 2.744059 seconds (Average: 0.914686 seconds)
fastgpx: 0.630138 seconds (Average: 0.210046 seconds)
```

### pybind11
```
> uv run benchmark_gpx.py

Computing expected distance with gpxpy...
gpxpy 5839236.226642554 meters
gpxpy: 13.327341 seconds

Running 3 benchmarks with 3 iterations...

xml_etree: 1.984600 seconds (Average: 0.661533 seconds)
lxml: 2.666522 seconds (Average: 0.888841 seconds)
fastgpx: 0.643969 seconds (Average: 0.214656 seconds)
```


## Polyline encode/decode

### nanobind
```
> uv run benchmark_polyline.py
GPX path: ../gpx/2024 Great Roadtrip
GPX files: 24
Iterations: 10

benchmarking fastgpx.polyline.encode
2.988260000005539

benchmarking polyline.encode
7.145514600000752
```

# pybind11
```
> uv run benchmark_polyline.py
GPX path: ../gpx/2024 Great Roadtrip
GPX files: 24
Iterations: 10

benchmarking fastgpx.polyline.encode
4.028022899998177

benchmarking polyline.encode
9.589258299994981
```
