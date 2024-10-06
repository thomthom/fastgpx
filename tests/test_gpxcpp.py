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


def test_process_string():
    value = gpxcpp.process_string("Hello, World!")
    assert value == "Processed: Hello, World!"

# tinyxml length3d


def test_tinyxml_gpx_length3d(gpx, gpx_path):
    distance = gpxcpp.tinyxml_gpx_length(gpx_path)
    expected = gpx.length_3d()
    assert distance == pytest.approx(expected)

# pugixml length3d


def test_pugixml_gpx_length3d(gpx, gpx_path):
    distance = gpxcpp.pugixml_gpx_length(gpx_path)
    expected = gpx.length_3d()
    assert distance == pytest.approx(expected)


def test_pugixml_track_length3d(gpx, gpx_path):
    expected = gpx.tracks[0].length_3d()
    distance = gpxcpp.pugixml_gpx_length(gpx_path)
    assert distance == pytest.approx(expected)


def test_pugixml_segment_length3d(gpx, gpx_path):
    expected = gpx.tracks[0].segments[0].length_3d()
    distance = gpxcpp.pugixml_gpx_length(gpx_path)
    assert distance == pytest.approx(expected)

# pugixml length2d


def test_pugixml_gpx_length2d(gpx, gpx_path):
    distance = gpxcpp.pugixml_gpx_length(gpx_path)
    expected = gpx.length_2d()
    assert distance == pytest.approx(expected)


def test_pugixml_track_length2d(gpx, gpx_path):
    expected = gpx.tracks[0].length_2d()
    distance = gpxcpp.pugixml_gpx_length(gpx_path)
    assert distance == pytest.approx(expected)


def test_pugixml_segment_length2d(gpx, gpx_path):
    expected = gpx.tracks[0].segments[0].length_2d()
    distance = gpxcpp.pugixml_gpx_length(gpx_path)
    assert distance == pytest.approx(expected)

# fastgpx length2d


def test_fastgpx_gpx_length2d(gpx, gpx_path):
    distance = gpxcpp.fastgpx_gpx_length(gpx_path)
    expected = gpx.length_2d()
    assert distance == pytest.approx(expected)


def test_fastgpx_track_length2d(gpx, gpx_path):
    expected = gpx.tracks[0].length_2d()
    distance = gpxcpp.fastgpx_track_length(gpx_path, 0)
    assert distance == pytest.approx(expected)


def test_fastgpx_segment_length2d(gpx, gpx_path):
    expected = gpx.tracks[0].segments[0].length_2d()
    distance = gpxcpp.fastgpx_segment_length(gpx_path, 0, 0)
    assert distance == pytest.approx(expected)
