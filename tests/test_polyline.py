import fastgpx

from pathlib import Path

import gpxpy
import polyline
import pytest


@pytest.fixture
def gpx_path():
    return "gpx/2024 TopCamp/Connected_20240518_094959_.gpx"


@pytest.fixture
def expected_gpx(gpx_path):
    with open(gpx_path, 'r', encoding='utf-8') as gpx_file:
        gpx = gpxpy.parse(gpx_file)
    return gpx


def test_encode_segment(gpx_path):
    gpx = fastgpx.parse(gpx_path)
    points = [(point.latitude, point.longitude)
              for point in gpx.tracks[0].segments[0].points]

    expected = polyline.encode(points, precision=6)
    actual = fastgpx.polyline.encode(gpx.tracks[0].segments[0].points)
    assert actual == expected


def test_decode_segment(expected_gpx, gpx_path):
    gpx = fastgpx.parse(gpx_path)
    points = [(point.latitude, point.longitude)
              for point in gpx.tracks[0].segments[0].points]
    polyline6 = polyline.encode(points, precision=6)

    expected = polyline.decode(polyline6, precision=6)
    result = fastgpx.polyline.decode(polyline6)
    actual = [(point.latitude, point.longitude) for point in result]
    assert actual == expected
