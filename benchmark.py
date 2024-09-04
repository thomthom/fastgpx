import math
import os
import timeit
import xml.etree.ElementTree as ET
from math import radians, sin, cos, sqrt, atan2

import gpxpy
from gpxpy.geo import EARTH_RADIUS
from gpxpy.geo import length_2d, length_3d


# # Haversine formula to calculate the distance between two GPS coordinates
# # https://github.com/jedie/gpxpy/blob/b371de31826cfecc049a57178d168b04a7f6d0d8/gpxpy/geo.py#L38
# def haversine(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
#     # R = 6371.0  # Radius of the Earth in kilometers
#     # R = 6371000.0  # Radius of the Earth in meters
#     # https://github.com/jedie/gpxpy/commit/b371de31826cfecc049a57178d168b04a7f6d0d8
#     # EARTH_RADIUS = 6378.137 * 1000
#     R = EARTH_RADIUS
#     lat1_rad = radians(lat1)
#     lon1_rad = radians(lon1)
#     lat2_rad = radians(lat2)
#     lon2_rad = radians(lon2)

#     dlat = lat2_rad - lat1_rad
#     dlon = lon2_rad - lon1_rad

#     a = sin(dlat / 2) ** 2 + cos(lat1_rad) * cos(lat2_rad) * sin(dlon / 2) ** 2
#     c = 2 * atan2(sqrt(a), sqrt(1 - a))

#     return R * c  # Distance in kilometers

# WGS84 Earth radius in meters (same as gpxpy)
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


def calculate_gpx_length(file_path: str) -> float:
    tree = ET.parse(file_path)
    root = tree.getroot()

    # Namespace handling for GPX
    ns = {'gpx': 'http://www.topografix.com/GPX/1/1'}

    total_distance = 0.0
    prev_point = None

    # Find all track points ('trkpt')
    for trkpt in root.findall('.//gpx:trkpt', ns):
        lat = float(trkpt.attrib['lat'])
        lon = float(trkpt.attrib['lon'])

        if prev_point is not None:
            total_distance += haversine(prev_point[0], prev_point[1], lat, lon)
            # total_distance += length_2d((prev_point[0], prev_point[1]), (lat, lon))

        prev_point = (lat, lon)

    return total_distance


def read_xml_etree():
    total_length = 0.0
    gpx_files = get_gpx_files(GPX_PATH)
    for gpx_filepath in gpx_files:
        length = calculate_gpx_length(gpx_filepath)
        total_length += length
    print('xml_etree', total_length, 'meters')
    return total_length


benchmarks = [
    {'name': 'gpxpy', 'function': read_gpxpy},
    {'name': 'xml_etree', 'function': read_xml_etree},
]

iterations = 1
print(f'Running {len(benchmarks)} benchmarks with {iterations} iterations...')
for benchmark in benchmarks:
    func = benchmark['function']
    print(f"Running {benchmark['name']} ...")
    execution_time = timeit.timeit(func, number=iterations)
    average_time = execution_time / iterations
    print(f"{benchmark['name']}: {execution_time:.6f} seconds (Average: {average_time:.6f} seconds)")
