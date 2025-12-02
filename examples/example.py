from fastgpx import LatLong
from fastgpx import polyline, geo
from fastgpx.polyline import Precision

locations = [
    LatLong(64, 10),  # This works. Types and functions are accessible.
    LatLong(66, 11),
]

encoded1 = polyline.encode(locations, precision=polyline.Precision.Six)
decoded1 = polyline.decode(encoded1, precision=polyline.Precision.Six)
print("Encoded:", encoded1)
print("Decoded:", decoded1)
print()

encoded2 = polyline.encode(locations, precision=Precision.Six)
decoded2 = polyline.decode(encoded2, precision=Precision.Six)
print("Encoded:", encoded2)
print("Decoded:", decoded2)
print()

distance = geo.haversine(locations[0], locations[1])
print("Distance:", distance)
print()
