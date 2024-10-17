import fastgpx

from pathlib import Path

import gpxpy
import pytest


@pytest.fixture
def gpx_path():
    return "gpx/2024 TopCamp/Connected_20240518_094959_.gpx"


@pytest.fixture
def expected_gpx(gpx_path):
    with open(gpx_path, 'r', encoding='utf-8') as gpx_file:
        gpx = gpxpy.parse(gpx_file)
    return gpx


def test_simple_segment_length2d():
    path = Path('gpx/test/debug-segment.gpx')
    with open(path, 'r', encoding='utf-8') as gpx_file:
        expected_gpx = gpxpy.parse(gpx_file)
    expected = expected_gpx.tracks[0].segments[0].length_2d()

    gpx = fastgpx.parse(path)
    # Assigning intermediate values for easier debug inspection.
    tracks = gpx.tracks
    track = tracks[0]
    segments = track.segments
    segment = segments[0]
    distance = segment.length_2d()
    assert distance == pytest.approx(expected)


def test_gpx_length2d(expected_gpx, gpx_path):
    expected = expected_gpx.length_2d()
    gpx = fastgpx.parse(gpx_path)
    distance = gpx.length_2d()
    assert distance == pytest.approx(expected)


def test_track_length2d(expected_gpx, gpx_path):
    expected = expected_gpx.tracks[0].length_2d()
    gpx = fastgpx.parse(gpx_path)
    distance = gpx.tracks[0].length_2d()
    assert distance == pytest.approx(expected)


def test_segment_length2d(expected_gpx, gpx_path):
    expected = expected_gpx.tracks[0].segments[0].length_2d()
    gpx = fastgpx.parse(gpx_path)
    distance = gpx.tracks[0].segments[0].length_2d()
    assert distance == pytest.approx(expected)
