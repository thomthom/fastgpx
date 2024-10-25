import gpxpy
import gpxpy.gpx
import pytest

import gpxcpp

METERS_TOL = 1e-4


@pytest.fixture
def gpx_path():
    return "gpx/2024 TopCamp/Connected_20240518_094959_.gpx"


@pytest.fixture
def gpx(gpx_path: str):
    with open(gpx_path, 'r', encoding='utf-8') as gpx_file:
        gpx = gpxpy.parse(gpx_file)
    return gpx


@pytest.mark.gpxcompare
def test_process_string():
    value = gpxcpp.process_string("Hello, World!")
    assert value == "Processed: Hello, World!"

# tinyxml length2d


@pytest.mark.gpxcompare
def test_tinyxml_gpx_length2d(gpx: gpxpy.gpx.GPX, gpx_path: str):
    distance = gpxcpp.tinyxml_gpx_length2d(gpx_path)
    expected = gpx.length_2d()
    assert distance == pytest.approx(expected)

# pugixml length2d


@pytest.mark.gpxcompare
def test_pugixml_gpx_length2d(gpx: gpxpy.gpx.GPX, gpx_path: str):
    distance = gpxcpp.pugixml_gpx_length2d(gpx_path)
    expected = gpx.length_2d()
    assert distance == pytest.approx(expected)

# fastgpx length2d


@pytest.mark.gpxcompare
def test_fastgpx_gpx_length2d(gpx: gpxpy.gpx.GPX, gpx_path: str):
    distance = gpxcpp.fastgpx_gpx_length2d(gpx_path)
    expected = gpx.length_2d()
    assert distance == pytest.approx(382952.7193, abs=METERS_TOL)


@pytest.mark.gpxcompare
def test_fastgpx_track_length2d(gpx: gpxpy.gpx.GPX, gpx_path: str):
    expected = gpx.tracks[0].length_2d()
    distance = gpxcpp.fastgpx_track_length2d(gpx_path, 0)
    assert distance == pytest.approx(382952.71935, abs=METERS_TOL)


@pytest.mark.gpxcompare
def test_fastgpx_segment_length2d(gpx: gpxpy.gpx.GPX, gpx_path: str):
    expected = gpx.tracks[0].segments[0].length_2d()
    distance = gpxcpp.fastgpx_segment_length2d(gpx_path, 0, 0)
    assert distance == pytest.approx(17809.2701, abs=METERS_TOL)

# single segment


@pytest.mark.gpxcompare
def test_fastgpx_two_point_segment_length():
    path = 'gpx/test/two-points.gpx'
    distance = gpxcpp.fastgpx_segment_length2d(path, 0, 0)
    assert distance == pytest.approx(261161.1785, abs=METERS_TOL)


@pytest.mark.gpxcompare
def test_fastgpx_segment_length():
    path = 'gpx/test/segment.gpx'
    distance = gpxcpp.fastgpx_segment_length2d(path, 0, 0)
    assert distance == pytest.approx(17809.2701, abs=METERS_TOL)


@pytest.mark.gpxcompare
def test_fastgpx_debug_segment_length():
    path = 'gpx/test/debug-segment.gpx'
    distance = gpxcpp.fastgpx_segment_length2d(path, 0, 0)
    assert distance == pytest.approx(1.3839, abs=METERS_TOL)
