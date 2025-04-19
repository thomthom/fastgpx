file:///C:/Users/Thomas/SourceTree/route-map/frame.html?width=800&height=600


```sh
flask --app server run --debug --port=5566
```

https://github.com/JerckyLY/mapboxgl-print-tool
https://github.com/watergis/maplibre-gl-export/tree/main/packages/mapbox-gl-export

https://www.mapbox.com/impact-tools/print-maps
https://blog.mapbox.com/designing-the-vintage-style-in-mapbox-studio-9da4aa2a627f

# GPX/XML Performance

Loading GPX files isn't fast. Cache meta data.

gpxpy docs says the lxml is faster. It is used automatically if available.
When testing that seemed not to be the case. Maybe stdlib xml lib has gotten faster?

## Python

### Hal9000

#### Without `lxml`
```
Running 2 benchmarks with 3 iterations...
Running gpxpy ...
gpxpy 5463041.784135511 meters
gpxpy 5463041.784135511 meters
gpxpy 5463041.784135511 meters
gpxpy: 11.497863 seconds (Average: 3.832621 seconds)
Running xml_etree ...
xml_etree 5463043.740615641 meters
xml_etree 5463043.740615641 meters
xml_etree 5463043.740615641 meters
xml_etree: 2.538976 seconds (Average: 0.846325 seconds)
```

#### With `lxml`
```
Running 2 benchmarks with 3 iterations...
Running gpxpy ...
gpxpy 5463041.784135511 meters
gpxpy 5463041.784135511 meters
gpxpy 5463041.784135511 meters
gpxpy: 37.803625 seconds (Average: 12.601208 seconds)
Running xml_etree ...
xml_etree 5463043.740615641 meters
xml_etree 5463043.740615641 meters
xml_etree 5463043.740615641 meters
xml_etree: 2.333200 seconds (Average: 0.777733 seconds)
```

## C++

### Hal9000
```
GPX Reader
C:\Users\Thomas\SourceTree\route-map\gpx\2024 Great Roadtrip

tinyxml2
Total Length: 5456930.710560566
Elapsed time: 0.4980144 seconds

pugixml
Total Length: 5456930.710560566
Elapsed time: 0.1890089 seconds
```

### Surface ARM64
```
GPX Reader
C:\Users\thoma\Source\gpx-maps\gpx\2024 Great Roadtrip

tinyxml2
Total Length: 5456930.710560566
Elapsed time: 0.4957163 seconds

pugixml
Total Length: 5456930.710560566
Elapsed time: 0.2605081 seconds
```

```
Running 5 benchmarks with 3 iterations...

Running gpxpy ...
gpxpy 5463041.784135511 meters
gpxpy 5463041.784135511 meters
gpxpy 5463041.784135511 meters
gpxpy: 50.182288 seconds (Average: 16.727429 seconds)

Running xml_etree ...
xml_etree 5463043.740615641 meters
xml_etree 5463043.740615641 meters
xml_etree 5463043.740615641 meters
xml_etree: 8.269050 seconds (Average: 2.756350 seconds)

Running lxml ...
lxml 5463043.740615641 meters
lxml 5463043.740615641 meters
lxml 5463043.740615641 meters
lxml: 8.479702 seconds (Average: 2.826567 seconds)

Running tinyxml (C++) ...
tinyxml 5456930.710560566 meters
tinyxml 5456930.710560566 meters
tinyxml 5456930.710560566 meters
tinyxml (C++): 2.699880 seconds (Average: 0.899960 seconds)

Running pugixml (C++) ...
pugixml 2481753.2382055665 meters
pugixml 2481753.2382055665 meters
pugixml 2481753.2382055665 meters
pugixml (C++): 0.381095 seconds (Average: 0.127032 seconds)
```

## Coverage

```sh
coverage.bat ~[real_world]
```

# Python C Extension

Once set up, you can build the C++ extension with a simple `pip install .` or `pip install --editable .` for development builds.

```sh
pip install --editable .
```


```sh
pybind11-stubgen gpxcpp -o .
```

```sh
pybind11-stubgen fastgpx -o .

pybind11-stubgen fastgpx --enum-class-locations "Precision:fastgpx.polyline" -o .
```

# Python Profiling

https://learn.microsoft.com/en-us/visualstudio/python/profiling-python-code-in-visual-studio?view=vs-2022

```sh
snakeviz profiling/fastgpx_polyline_encode.prof
```

```sh
snakeviz profiling/polyline_encode.prof
```

# pyproject.toml

> Installing Dependencies with pyproject.toml
>
> You no longer need to use `pip install -r requirements.txt`. Instead, you can simply install dependencies directly using:
>
> ```sh
> pip install .
> ```

> For Development Dependencies:
>
> To install development dependencies (like `pytest` and `pybind11-stubgen`), you can use the `--extra` option (assuming you defined them under dev):
>
> ```sh
> pip install .[dev]
> ```
>
> This will install both the main dependencies and the development dependencies defined in the dev section of `pyproject.toml`.

# Catch2 Tests

If the output prints UTF-8 characters the terminal needs to be set to UTF-8 mode on Windows:

```sh
chcp 65001
```

```sh
build\cpp\RelWithDebInfo\fastgpx_test.exe [!benchmark]
```

# GPX

## GPX for Developers

https://www.topografix.com/gpx_for_developers.asp

> The standard in the gpx files for the time zone format is not completely
> clear. For instance some applications generate time formats in the track
> points of the form
>
> <time>2008-07-18T16:07:50.000+02:00</time>
>
> this is slightly substandard because the standard as described in
> http://www.topografix.com/gpx.asp says that
>
> "Date and time in are in Univeral Coordinated Time (UTC), not local time!
> Conforms to ISO 8601 specification for date/time representation."

https://web.archive.org/web/20130725164436/http://tech.groups.yahoo.com/group/gpsxml/message/1090?l=1

## GPS Exchange Format (GPX): A Comprehensive Guide

https://mapscaping.com/gps-exchange-format-gpx/

## Assumed typical <time> representation in .GPX files

To simplify logic and maximize performance, maybe one can assume a more narrow
scope of ISO 8601.

By using the length of the string one can make assumption to the variation of
the format and directly extract the numeric components of the string. Probably
worth validating the separator characters as a minimal validation safety check
to eliminate pure junk input.

Variations observed:
* Only Extended Format.
* With or without milliseconds. (3 fractional decimals)
* Zulu hours are the norm, but timezone offsets might occur.
* Some mention of missing timezone notation all together.
  While ISO 8601 describe this as local time, one can probably assume this is
  an omission by the author and it really should be Zulu hours.

| Example                       | Length |                                     |
|-------------------------------|--------|-------------------------------------|
| 2008-07-18T16:07:50.000+02:00 |     29 | Not per GPX definition.             |
| 2008-07-18T16:07:50.000Z      |     24 |                                     |
| 2008-07-18T16:07:50+02:00     |     25 | Not per GPX definition.             |
| 2008-07-18T16:07:50Z          |     20 |                                     |
| 2008-07-18T16:07:50           |     19 | Assume Zulu time?                   |
