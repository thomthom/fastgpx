import math
import os
import timeit
import xml.etree.ElementTree as ET
from math import radians, sin, cos, sqrt, atan2

import gpxpy
from gpxpy.geo import EARTH_RADIUS
from gpxpy.geo import length_2d, length_3d, Location

from lxml import etree

import gpxcpp

# https://github.com/jedie/gpxpy/commit/b371de31826cfecc049a57178d168b04a7f6d0d8
# https://github.com/jedie/gpxpy/blob/b371de31826cfecc049a57178d168b04a7f6d0d8/gpxpy/geo.py#L38
# https://github.com/jedie/gpxpy/blob/b371de31826cfecc049a57178d168b04a7f6d0d8/gpxpy/geo.py#L166
EARTH_RADIUS = 6378.137 * 1000


def to_rad(x: float) -> float:
    return x / 180.0 * math.pi


def haversine(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    d_lat = to_rad(lat2 - lat1)
    d_lon = to_rad(lon2 - lon1)
    lat1_rad = to_rad(lat1)
    lat2_rad = to_rad(lat2)

    a = math.sin(d_lat / 2) ** 2 + math.cos(lat1_rad) * \
        math.cos(lat2_rad) * math.sin(d_lon / 2) ** 2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    return EARTH_RADIUS * c  # Distance in meters


def get_gpx_files(path: str):
    gpx_files = []
    files = os.listdir(path)
    for filename in files:
        if filename.endswith(".gpx"):
            gpx_path = os.path.join(path, filename)
            gpx_files.append(gpx_path)
    return gpx_files


GPX_PATH = 'gpx/2024 Great Roadtrip'

# gpxpy


def read_gpxpy():
    total_length = 0.0
    gpx_files = get_gpx_files(GPX_PATH)
    for gpx_filepath in gpx_files:
        with open(gpx_filepath, 'r', encoding='utf-8') as gpx_file:
            gpx = gpxpy.parse(gpx_file)
            total_length += gpx.length_2d()
            # total_length += gpx.length_3d()
    print('gpxpy', total_length, 'meters')
    return total_length

# xml.etree


def xml_etree_calculate_segment_length(trkseg) -> float:
    total_distance = 0.0
    prev_point = None

    ns = {'gpx': 'http://www.topografix.com/GPX/1/1'}
    for trkpt in trkseg.findall('.//gpx:trkpt', ns):
        lat = float(trkpt.attrib['lat'])
        lon = float(trkpt.attrib['lon'])

        if prev_point is not None:
            total_distance += haversine(prev_point[0], prev_point[1], lat, lon)

        prev_point = (lat, lon)

    return total_distance


def xml_etree_calculate_gpx_length(file_path: str) -> float:
    tree = ET.parse(file_path)
    root = tree.getroot()

    # Namespace for GPX 1.1
    ns = {'gpx': 'http://www.topografix.com/GPX/1/1'}

    total_distance = 0.0

    # Iterate over all tracks (trk)
    for trk in root.findall('.//gpx:trk', ns):
        # Each track can have multiple track segments (trkseg)
        for trkseg in trk.findall('.//gpx:trkseg', ns):
            total_distance += xml_etree_calculate_segment_length(trkseg)

    return total_distance


def read_xml_etree():
    total_length = 0.0
    gpx_files = get_gpx_files(GPX_PATH)
    for gpx_filepath in gpx_files:
        length = xml_etree_calculate_gpx_length(gpx_filepath)
        total_length += length
    print('xml_etree', total_length, 'meters')
    return total_length


# lxml


def lxml_calculate_segment_length(trkseg) -> float:
    total_distance = 0.0
    prev_point = None

    # Find all track points in this segment
    for trkpt in trkseg.findall('.//{http://www.topografix.com/GPX/1/1}trkpt'):
        lat = float(trkpt.attrib['lat'])
        lon = float(trkpt.attrib['lon'])

        if prev_point is not None:
            total_distance += haversine(prev_point[0], prev_point[1], lat, lon)

        prev_point = (lat, lon)

    return total_distance

# Function to calculate total length of GPX file, considering multiple track segments


def lxml_calculate_gpx_length(file_path: str) -> float:
    tree = etree.parse(file_path, None)
    root = tree.getroot()

    total_distance = 0.0

    # Iterate over all tracks (trk)
    for trk in root.findall('.//{http://www.topografix.com/GPX/1/1}trk'):
        # Each track can have multiple track segments (trkseg)
        for trkseg in trk.findall('.//{http://www.topografix.com/GPX/1/1}trkseg'):
            total_distance += lxml_calculate_segment_length(trkseg)

    return total_distance


def read_lxml():
    total_length = 0.0
    gpx_files = get_gpx_files(GPX_PATH)
    for gpx_filepath in gpx_files:
        length = lxml_calculate_gpx_length(gpx_filepath)
        total_length += length
    print('lxml', total_length, 'meters')
    return total_length


def read_tinyxml():
    total_length = 0.0
    gpx_files = get_gpx_files(GPX_PATH)
    for gpx_filepath in gpx_files:
        # print(f'gpx_filepath: {gpx_filepath}')
        fullpath = os.path.abspath(gpx_filepath)
        # print(f'fullpath: {fullpath}')

        length = gpxcpp.tinyxml_gpx_length(fullpath)

        # gpxdata = open(fullpath).read()
        # length = gpxcpp.tinyxml_gpx_length(gpxdata)

        total_length += length
    print('tinyxml', total_length, 'meters')
    return total_length


def read_pugixml():
    total_length = 0.0
    gpx_files = get_gpx_files(GPX_PATH)
    for gpx_filepath in gpx_files:
        length = gpxcpp.pugixml_gpx_length(gpx_filepath)
        total_length += length
    print('pugixml', total_length, 'meters')
    return total_length

# Benchmarks:


benchmarks = [
    # {'name': 'xml_etree', 'function': read_xml_etree},
    # {'name': 'lxml', 'function': read_lxml},
    # {'name': 'gpxpy', 'function': read_gpxpy},
    {'name': 'tinyxml (C++)', 'function': read_tinyxml},
    {'name': 'pugixml (C++)', 'function': read_pugixml},
]

print(gpxcpp.process_string('Hi C Extension'))

print(f"Python script PID: {os.getpid()}")
# print("Press Enter to continue...")
# input()

iterations = 3
print(f'Running {len(benchmarks)} benchmarks with {iterations} iterations...')
for benchmark in benchmarks:
    func = benchmark['function']
    print()
    print(f"Running {benchmark['name']} ...")
    execution_time = timeit.timeit(func, number=iterations)
    average_time = execution_time / iterations
    print(f"{benchmark['name']}: {execution_time:.6f} seconds (Average: {average_time:.6f} seconds)")
