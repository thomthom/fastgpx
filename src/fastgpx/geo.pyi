import fastgpx


def haversine(latlong1: fastgpx.LatLong, latlong2: fastgpx.LatLong) -> float:
    """Haversine distance returned in meters using ``osmium`` logic."""

def haversine_distance(latitude_1: float, longitude_1: float, latitude_2: float, longitude_2: float) -> float:
    """
    .. warning::

       Signature compatibility with ``gpxpy.geo.haversine_distance``.
       Prefer :func:`haversine` instead.

    .. important::

       While the signature matches ``gpxpy``, the implementation uses ``osmium`` logic    which may lead to slightly different results.
    """
