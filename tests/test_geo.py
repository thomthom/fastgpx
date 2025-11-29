import pytest

import fastgpx

METERS_TOL = 1e-4


class TestGeo:

    # fastgpx.geo.haversine

    def test_haversine(self):
        path = 'gpx/test/debug-segment.gpx'
        gpx = fastgpx.parse(path)
        points = gpx.tracks[0].segments[0].points
        assert len(points) == 2
        point1 = points[0]
        point2 = points[1]
        distance = fastgpx.geo.haversine(point1, point2)
        assert distance == pytest.approx(1.3839, abs=METERS_TOL)

    def test_haversine_distance(self):
        path = 'gpx/test/debug-segment.gpx'
        gpx = fastgpx.parse(path)
        points = gpx.tracks[0].segments[0].points
        assert len(points) == 2
        point1 = points[0]
        point2 = points[1]
        lat1 = point1.latitude
        lon1 = point1.longitude
        lat2 = point2.latitude
        lon2 = point2.longitude
        distance = fastgpx.geo.haversine_distance(lat1, lon1, lat2, lon2)
        assert distance == pytest.approx(1.3839, abs=METERS_TOL)
