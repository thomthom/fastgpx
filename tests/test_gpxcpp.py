import gpxcpp

import gpxpy
import pytest


def get_gpx_length(gpx_path):
    with open(gpx_path, 'r', encoding='utf-8') as gpx_file:
        gpx = gpxpy.parse(gpx_file)
    return gpx.length_3d()


def test_example():
    assert 1 + 1 == 2


def test_hello():
    value = gpxcpp.process_string("Hello, World!")
    assert value == "Processed: Hello, World!"


def test_tinyxml_gpx_length():
    gpx_path = "gpx/2024 TopCamp/Connected_20240518_094959_.gpx"
    distance = gpxcpp.tinyxml_gpx_length(gpx_path)
    expected = get_gpx_length(gpx_path)
    assert distance == pytest.approx(expected)


def test_pugixml_gpx_length():
    gpx_path = "gpx/2024 TopCamp/Connected_20240518_094959_.gpx"
    distance = gpxcpp.pugixml_gpx_length(gpx_path)
    expected = get_gpx_length(gpx_path)
    assert distance == pytest.approx(expected)
