import datetime

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

# fastgpx.Gpx.length_2d


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

# fastgpx.Gpx.bounds


def test_gpx_bounds(gpx_path: str):
    gpx = fastgpx.parse(gpx_path)
    bounds = gpx.bounds()
    assert not bounds.is_empty()
    assert bounds.min is not None
    assert bounds.min.latitude == pytest.approx(61.410713)
    assert bounds.min.longitude == pytest.approx(10.427408)
    assert bounds.max is not None
    assert bounds.max.latitude == pytest.approx(63.441189)
    assert bounds.max.longitude == pytest.approx(13.142774)

# fastgpx.Gpx.time_bounds


def test_gpx_time_bounds(gpx_path: str):
    gpx = fastgpx.parse(gpx_path)
    time_bounds = gpx.time_bounds()
    assert not time_bounds.is_empty()

    # 2024-05-18T07:50:00Z
    assert time_bounds.start_time is not None
    assert time_bounds.start_time == datetime.datetime(
        year=2024, month=5, day=18, hour=7, minute=50, second=0, tzinfo=datetime.timezone.utc)
    assert time_bounds.start_time.tzinfo is datetime.timezone.utc
    assert time_bounds.start_time.isoformat() == '2024-05-18T07:50:00+00:00'
    assert time_bounds.start_time.timestamp() == 1716018600

    # 2024-05-18T16:46:18Z
    assert time_bounds.end_time is not None
    assert time_bounds.end_time == datetime.datetime(
        year=2024, month=5, day=18, hour=16, minute=46, second=18, tzinfo=datetime.timezone.utc)
    assert time_bounds.end_time.tzinfo is datetime.timezone.utc
    assert time_bounds.end_time.isoformat() == '2024-05-18T16:46:18+00:00'
    assert time_bounds.end_time.timestamp() == 1716050778
