import fastgpx
from fastgpx import polyline
from fastgpx.fastgpx.polyline import encode

Precision = polyline.Precision

locations = [
    fastgpx.LatLong(64, 10),
    fastgpx.LatLong(66, 11),
]
x = encode(locations)
y = encode(locations, precision=6)
z = encode(locations, precision=Precision.Six)
