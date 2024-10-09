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
