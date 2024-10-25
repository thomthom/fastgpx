import gpxpy
import gpxpy.gpx
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


METERS_TOL = 1e-4


def test_simple_segment_length2d():
    path = 'gpx/test/debug-segment.gpx'
    gpx = fastgpx.parse(path)
    # Assigning intermediate values for easier debug inspection.
    tracks = gpx.tracks
    track = tracks[0]
    segments = track.segments
    segment = segments[0]
    distance = segment.length_2d()
    assert distance == pytest.approx(1.3839, abs=METERS_TOL)


def test_gpx_length2d(gpx_path: str):
    gpx = fastgpx.parse(gpx_path)
    distance = gpx.length_2d()
    assert distance == pytest.approx(382952.7193, abs=METERS_TOL)


def test_track_length2d(gpx_path: str):
    gpx = fastgpx.parse(gpx_path)
    distance = gpx.tracks[0].length_2d()
    assert distance == pytest.approx(382952.7193, abs=METERS_TOL)


def test_segment_length2d(gpx_path: str):
    gpx = fastgpx.parse(gpx_path)
    distance = gpx.tracks[0].segments[0].length_2d()
    assert distance == pytest.approx(17809.2701, abs=METERS_TOL)
