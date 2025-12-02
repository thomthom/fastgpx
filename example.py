import fastgpx

locations = [
    fastgpx.LatLong(64, 10),
    fastgpx.LatLong(66, 11),
]
encoded = fastgpx.polyline.encode(locations, precision=6)

decoded = fastgpx.polyline.decode(encoded, precision=6)
