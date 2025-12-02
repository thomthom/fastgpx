import fastgpx
from fastgpx import polyline

locations = [
    fastgpx.LatLong(64, 10),
    fastgpx.LatLong(66, 11),
]
x = polyline.encode(locations)
y = polyline.encode(locations, precision=6)
z = polyline.encode(locations, precision=polyline.Precision.Six)
