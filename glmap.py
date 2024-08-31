import gpxpy


with open('map.html', mode='r') as file:
    html_template = file.read()

gpx_filepath = 'gpx/2024 Great Roadtrip/Connected_20240806_102330_Holmboevegen_8_9010_Tromsø.gpx'

with open(gpx_filepath, 'r') as gpx_file:
    gpx = gpxpy.parse(gpx_file)

bounds = gpx.get_bounds()
assert bounds is not None
# [west, south, east, north]
bounds_js = f'[{bounds.min_longitude},{bounds.min_latitude},{bounds.max_longitude},{bounds.max_latitude}]'

coords = []
for tracks in gpx.tracks:
    for segments in tracks.segments:
        for point in segments.points:
            # long, lat
            coords.append(f'[{point.longitude},{point.latitude}]')
coords_js = ",\n              ".join(coords)

html = html_template.replace('%BOUNDS%', bounds_js)
html = html.replace('%COORDS%', coords_js)

with open('output/map.html', mode='w') as file:
    file.write(html)
