from __future__ import annotations
import fastgpx.fastgpx
__all__ = ['haversine', 'haversine_distance']
def haversine(latlong1: fastgpx.fastgpx.LatLong, latlong2: fastgpx.fastgpx.LatLong) -> float:
    ...
def haversine_distance(latitude_1: float, longitude_1: float, latitude_2: float, longitude_2: float) -> float:
    """
    Compatibility with `gpxpy.geo.haversine_distance`
    """
