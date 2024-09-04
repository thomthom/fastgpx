file:///C:/Users/Thomas/SourceTree/route-map/frame.html?width=800&height=600


```sh
flask --app server run --debug
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
