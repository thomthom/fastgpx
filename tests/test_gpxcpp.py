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


def test_tinyxml_gpx_length3d(gpx, gpx_path):
    distance = gpxcpp.tinyxml_gpx_length(gpx_path)
    expected = gpx.length_3d()
    assert distance == pytest.approx(expected)


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
