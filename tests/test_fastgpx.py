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
    assert bounds.is_valid()
    assert not bounds.is_empty()
    assert bounds.min is not None
    assert bounds.min.latitude == pytest.approx(61.410713)
    assert bounds.min.longitude == pytest.approx(10.427408)
    assert bounds.max is not None
    assert bounds.max.latitude == pytest.approx(63.441189)
    assert bounds.max.longitude == pytest.approx(13.142774)

# fastgpx.Bounds


def test_bounds_defaults():
    bounds = fastgpx.Bounds()
    assert not bounds.is_valid()
    assert bounds.is_empty()
    assert bounds.min is None
    assert bounds.min is None
    assert bounds.max is None
    assert bounds.max is None


def test_bounds_with_latlongs():
    ll1 = fastgpx.LatLong(-10, -5)
    ll2 = fastgpx.LatLong(30, 25)
    bounds = fastgpx.Bounds(ll1, ll2)
    assert bounds.is_valid()
    assert not bounds.is_empty()
    assert bounds.min is not None
    assert bounds.max is not None

    assert bounds.min.latitude == -10.0
    assert bounds.min.longitude == -5.0
    assert bounds.max.latitude == 30.0
    assert bounds.max.longitude == 25.0

    assert bounds.min.latitude == bounds.min_latitude
    assert bounds.min.longitude == bounds.min_longitude
    assert bounds.max.latitude == bounds.max_latitude
    assert bounds.max.longitude == bounds.max_longitude


def test_bounds_with_tuples():
    bounds = fastgpx.Bounds((-10, -5), (30, 25))
    assert bounds.is_valid()
    assert not bounds.is_empty()
    assert bounds.min is not None
    assert bounds.min.latitude == -10.0
    assert bounds.min.longitude == -5.0
    assert bounds.max is not None
    assert bounds.max.latitude == 30.0
    assert bounds.max.longitude == 25.0


def test_bounds_max_bounds():
    bounds1 = fastgpx.Bounds((-10, -5), (30, 25))
    bounds2 = fastgpx.Bounds((5, 30), (40, 20))
    bounds = bounds1.max_bounds(bounds2)
    assert bounds.is_valid()
    assert not bounds.is_empty()
    assert bounds.min is not None
    assert bounds.min.latitude == -10.0
    assert bounds.min.longitude == -5.0
    assert bounds.max is not None
    assert bounds.max.latitude == 40.0
    assert bounds.max.longitude == 30.0
