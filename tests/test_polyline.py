import gpxpy
import polyline
import pytest

import fastgpx


@pytest.fixture
def gpx_path():
    return "gpx/2024 TopCamp/Connected_20240518_094959_.gpx"


@pytest.fixture
def expected_gpx(gpx_path: str):
    with open(gpx_path, 'r', encoding='utf-8') as gpx_file:
        gpx = gpxpy.parse(gpx_file)
    return gpx


class TestPolyline:

    # fastgpx.polyline.encode

    def test_encode_p5_segment(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        points = [(point.latitude, point.longitude)
                  for point in gpx.tracks[0].segments[0].points]

        expected = polyline.encode(points, precision=5)
        actual = fastgpx.polyline.encode(
            gpx.tracks[0].segments[0].points,
            precision=fastgpx.polyline.Precision.Five)
        assert actual == expected

    def test_encode_p6_segment(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        points = [(point.latitude, point.longitude)
                  for point in gpx.tracks[0].segments[0].points]

        expected = polyline.encode(points, precision=6)
        actual = fastgpx.polyline.encode(
            gpx.tracks[0].segments[0].points,
            precision=fastgpx.polyline.Precision.Six)
        assert actual == expected

    def test_encode_p5_segment_int_overload(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        points = [(point.latitude, point.longitude)
                  for point in gpx.tracks[0].segments[0].points]

        expected = polyline.encode(points, precision=5)
        actual = fastgpx.polyline.encode(
            gpx.tracks[0].segments[0].points, precision=5)
        assert actual == expected

    def test_encode_p6_segment_int_overload(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        points = [(point.latitude, point.longitude)
                  for point in gpx.tracks[0].segments[0].points]

        expected = polyline.encode(points, precision=6)
        actual = fastgpx.polyline.encode(
            gpx.tracks[0].segments[0].points, precision=6)
        assert actual == expected

    def test_encode_segment_int_overload_invalid_arguments(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        points = gpx.tracks[0].segments[0].points

        with pytest.raises(ValueError):
            fastgpx.polyline.encode(points, precision=4)

        with pytest.raises(ValueError):
            fastgpx.polyline.encode(points, precision=7)

    # fastgpx.polyline.decode

    def test_decode_p5_segment(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        points = [(point.latitude, point.longitude)
                  for point in gpx.tracks[0].segments[0].points]
        polyline6 = polyline.encode(points, precision=5)

        expected = polyline.decode(polyline6, precision=5)
        result = fastgpx.polyline.decode(polyline6,
                                         precision=fastgpx.polyline.Precision.Five)
        actual = [(point.latitude, point.longitude) for point in result]
        assert actual == expected

    def test_decode_p6_segment(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        points = [(point.latitude, point.longitude)
                  for point in gpx.tracks[0].segments[0].points]
        polyline6 = polyline.encode(points, precision=6)

        expected = polyline.decode(polyline6, precision=6)
        result = fastgpx.polyline.decode(polyline6,
                                         precision=fastgpx.polyline.Precision.Six)
        actual = [(point.latitude, point.longitude) for point in result]
        assert actual == expected

    def test_decode_p5_segment_int_overload(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        points = [(point.latitude, point.longitude)
                  for point in gpx.tracks[0].segments[0].points]
        polyline6 = polyline.encode(points, precision=5)

        expected = polyline.decode(polyline6, precision=5)
        result = fastgpx.polyline.decode(polyline6, precision=5)
        actual = [(point.latitude, point.longitude) for point in result]
        assert actual == expected

    def test_decode_p6_segment_int_overload(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        points = [(point.latitude, point.longitude)
                  for point in gpx.tracks[0].segments[0].points]
        polyline6 = polyline.encode(points, precision=6)

        expected = polyline.decode(polyline6, precision=6)
        result = fastgpx.polyline.decode(polyline6, precision=6)
        actual = [(point.latitude, point.longitude) for point in result]
        assert actual == expected

    def test_decode_segment_int_overload_invalid_arguments(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        points = [(point.latitude, point.longitude)
                  for point in gpx.tracks[0].segments[0].points]
        polyline6 = polyline.encode(points, precision=6)

        with pytest.raises(ValueError):
            fastgpx.polyline.decode(polyline6, precision=4)

        with pytest.raises(ValueError):
            fastgpx.polyline.decode(polyline6, precision=7)

    # fastgpx.polyline.decode (malformed input)

    def test_decode_empty(self):
        assert fastgpx.polyline.decode('') == []

    def test_decode_keyword_arguments(self):
        encoded = fastgpx.polyline.encode([fastgpx.LatLong(64, 10)], precision=6)
        decoded = fastgpx.polyline.decode(encoded=encoded, precision=6)
        assert decoded == [fastgpx.LatLong(64, 10)]

    def test_decode_truncated_raises(self):
        encoded = fastgpx.polyline.encode(
            [fastgpx.LatLong(64, 10), fastgpx.LatLong(66, 11)], precision=6)
        with pytest.raises(RuntimeError, match='unexpected end'):
            fastgpx.polyline.decode(encoded[:-1], precision=6)

    def test_decode_overlong_value_raises(self):
        # Eight continuation chunks would shift past 32 bits.
        with pytest.raises(RuntimeError, match='too long'):
            fastgpx.polyline.decode('________@_')

    def test_decode_invalid_character_raises(self):
        with pytest.raises(RuntimeError, match='invalid character'):
            fastgpx.polyline.decode('_p~iF~ps|U\x01')
