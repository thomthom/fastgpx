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


def test_single_segment_length2d():
    path = 'gpx/test/debug-segment.gpx'
    with open(path, 'r', encoding='utf-8') as gpx_file:
        expected_gpx = gpxpy.parse(gpx_file)
    expected = expected_gpx.tracks[0].segments[0].length_2d()

    path_like = Path(path)
    gpx = fastgpx.parse(path_like)
    # gpx = fastgpx.parse(path) # This works, but static analyzer complain.
    # Assigning intermediate values for easier debug inspection.
    tracks = gpx.tracks
    track = tracks[0]
    segments = track.segments
    segment = segments[0]
    distance = segment.length_2d()
    assert distance == pytest.approx(expected)
