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

    # fastgpx.polyline.encode/decode (coordinate range)

    @pytest.mark.parametrize('precision', [5, 6])
    def test_round_trip_extreme_coordinates(self, precision: int):
        points = [(-90.0, -180.0), (90.0, 180.0), (-90.0, -180.0)]

        expected = polyline.encode(points, precision=precision)
        encoded = fastgpx.polyline.encode(
            [fastgpx.LatLong(lat, lng) for lat, lng in points], precision=precision)
        assert encoded == expected

        decoded = fastgpx.polyline.decode(encoded, precision=precision)
        assert [(point.latitude, point.longitude) for point in decoded] == points

    @staticmethod
    def _encode_value(value: int) -> str:
        # Reference implementation of a single polyline value (delta) encoding.
        value = ~(value << 1) if value < 0 else (value << 1)
        encoded = ''
        while value >= 0x20:
            encoded += chr((0x20 | (value & 0x1f)) + 63)
            value >>= 5
        return encoded + chr(value + 63)

    def test_decode_out_of_range_raises(self):
        # The largest delta representable in 6 chunks (2^29 - 1) already exceeds 90 degrees.
        encoded = self._encode_value((1 << 29) - 1) + self._encode_value(0)
        with pytest.raises(RuntimeError, match='latitude out of range'):
            fastgpx.polyline.decode(encoded, precision=5)

        encoded = self._encode_value(0) + self._encode_value(-((1 << 29) - 1))
        with pytest.raises(RuntimeError, match='longitude out of range'):
            fastgpx.polyline.decode(encoded, precision=6)

    def test_decode_accumulator_overflow_raises(self):
        # Five max-size positive deltas sum past 2^31. A 32-bit accumulator would wrap to a
        # negative latitude instead of failing.
        encoded = (self._encode_value((1 << 29) - 1) + self._encode_value(0)) * 5
        with pytest.raises(RuntimeError, match='out of range'):
            fastgpx.polyline.decode(encoded, precision=5)
