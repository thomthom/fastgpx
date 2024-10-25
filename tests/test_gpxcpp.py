import gpxcpp

import gpxpy
import pytest


@pytest.fixture
def gpx_path():
    return "gpx/2024 TopCamp/Connected_20240518_094959_.gpx"


@pytest.fixture
def gpx(gpx_path):
    with open(gpx_path, 'r', encoding='utf-8') as gpx_file:
        gpx = gpxpy.parse(gpx_file)
    return gpx


@pytest.mark.gpxcompare
def test_process_string():
    value = gpxcpp.process_string("Hello, World!")
    assert value == "Processed: Hello, World!"

# tinyxml length2d


@pytest.mark.gpxcompare
def test_tinyxml_gpx_length2d(gpx, gpx_path):
    distance = gpxcpp.tinyxml_gpx_length2d(gpx_path)
    expected = gpx.length_2d()
    assert distance == pytest.approx(expected)

# pugixml length2d


@pytest.mark.gpxcompare
def test_pugixml_gpx_length2d(gpx, gpx_path):
    distance = gpxcpp.pugixml_gpx_length2d(gpx_path)
    expected = gpx.length_2d()
    assert distance == pytest.approx(expected)

# fastgpx length2d


@pytest.mark.gpxcompare
def test_fastgpx_gpx_length2d(gpx, gpx_path):
    distance = gpxcpp.fastgpx_gpx_length2d(gpx_path)
    expected = gpx.length_2d()
    assert distance == pytest.approx(expected)


@pytest.mark.gpxcompare
def test_fastgpx_track_length2d(gpx, gpx_path):
    expected = gpx.tracks[0].length_2d()
    distance = gpxcpp.fastgpx_track_length2d(gpx_path, 0)
    assert distance == pytest.approx(expected)


@pytest.mark.gpxcompare
def test_fastgpx_segment_length2d(gpx, gpx_path):
    expected = gpx.tracks[0].segments[0].length_2d()
    distance = gpxcpp.fastgpx_segment_length2d(gpx_path, 0, 0)
    assert distance == pytest.approx(expected)

# single segment


@pytest.mark.gpxcompare
def test_fastgpx_two_point_segment_length():
    path = 'gpx/test/two-points.gpx'
    with open(path, 'r', encoding='utf-8') as gpx_file:
        gpx = gpxpy.parse(gpx_file)
    expected = gpx.tracks[0].segments[0].length_2d()
    distance = gpxcpp.fastgpx_segment_length2d(path, 0, 0)
    assert distance == pytest.approx(expected)


@pytest.mark.gpxcompare
def test_fastgpx_segment_length():
    path = 'gpx/test/segment.gpx'
    with open(path, 'r', encoding='utf-8') as gpx_file:
        gpx = gpxpy.parse(gpx_file)
    expected = gpx.tracks[0].segments[0].length_2d()
    distance = gpxcpp.fastgpx_segment_length2d(path, 0, 0)
    assert distance == pytest.approx(expected)


@pytest.mark.gpxcompare
def test_fastgpx_debug_segment_length():
    path = 'gpx/test/debug-segment.gpx'
    with open(path, 'r', encoding='utf-8') as gpx_file:
        gpx = gpxpy.parse(gpx_file)
    expected = gpx.tracks[0].segments[0].length_2d()
    distance = gpxcpp.fastgpx_segment_length2d(path, 0, 0)
    assert distance == pytest.approx(expected)
