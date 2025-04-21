import math
import os
import time
import timeit
import xml.etree.ElementTree as ET
from math import radians, sin, cos, sqrt, atan2
from typing import Callable, TypedDict

import gpxpy
from gpxpy.geo import EARTH_RADIUS
from gpxpy.geo import length_2d, length_3d, Location

from lxml import etree

from colorama import Fore, Back, Style

import fastgpx

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


GPX_PATH = '../gpx/2024 Great Roadtrip'

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
    points = []
    ns = {'gpx': 'http://www.topografix.com/GPX/1/1'}
    for trkpt in trkseg.findall('.//gpx:trkpt', ns):
        lat = float(trkpt.attrib['lat'])
        lon = float(trkpt.attrib['lon'])
        points.append(Location(lat, lon))
    return length_2d(points)


def xml_etree_calculate_gpx_length(file_path: str) -> float:
    tree = ET.parse(file_path)
    root = tree.getroot()
    ns = {'gpx': 'http://www.topografix.com/GPX/1/1'}
    total_distance = 0.0
    for trk in root.findall('.//gpx:trk', ns):
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
    points = []
    for trkpt in trkseg.findall('.//{http://www.topografix.com/GPX/1/1}trkpt'):
        lat = float(trkpt.attrib['lat'])
        lon = float(trkpt.attrib['lon'])
        points.append(Location(lat, lon))
    return length_2d(points)


def lxml_calculate_gpx_length(file_path: str) -> float:
    tree = etree.parse(file_path, None)
    root = tree.getroot()

    total_distance = 0.0

    for trk in root.findall('.//{http://www.topografix.com/GPX/1/1}trk'):
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


def read_fastgpx():
    total_length = 0.0
    gpx_files = get_gpx_files(GPX_PATH)
    for gpx_filepath in gpx_files:
        fullpath = os.path.abspath(gpx_filepath)
        gpx = fastgpx.parse(fullpath)
        length = gpx.length_2d()
        total_length += length
    print('fastgpx', total_length, 'meters')
    return total_length

# Benchmarks:


class Benchmark(TypedDict):
    name: str
    function: Callable[[], float]


benchmarks: list[Benchmark] = [
    # {'name': 'gpxpy', 'function': read_gpxpy}, # Slow!
    {'name': 'xml_etree', 'function': read_xml_etree},
    {'name': 'lxml', 'function': read_lxml},
    {'name': 'fastgpx', 'function': read_fastgpx},
]

# print(f"Python script PID: {os.getpid()}")
# print("Press Enter to continue...")
# input()

print()
print('Computing expected distance with gpxpy...' + Fore.LIGHTMAGENTA_EX)
# gpxpy is _very_ slow, so omitting from doing multiple benchmark runs with it.
t = time.perf_counter()
expected = read_gpxpy()
d = time.perf_counter()
e = d - t
print(Fore.LIGHTYELLOW_EX + f'gpxpy: {e:.6f} seconds')
print()

iterations = 3
print(Fore.CYAN + f'Running {len(benchmarks)} benchmarks with {iterations} iterations...')
for benchmark in benchmarks:
    func = benchmark['function']
    print()
    # Benchmark timings:
    print(Fore.LIGHTYELLOW_EX + f"Running {benchmark['name']} ..." + Fore.LIGHTBLACK_EX)
    execution_time = timeit.timeit(func, number=iterations)
    average_time = execution_time / iterations
    print(Fore.LIGHTYELLOW_EX +
          f"{benchmark['name']}: {execution_time:.6f} seconds (Average: {average_time:.6f} seconds)" + Fore.YELLOW)
    # Print info about result accuracy:
    result = func()
    name = benchmark['name'].split(' ')[0]
    indent = ' ' * len(name)
    print(Fore.YELLOW +
          f"{indent} {expected} meters (expected)")
    deviation = result - expected
    percent = (deviation / expected) * 100
    print(Fore.LIGHTMAGENTA_EX + f'Distance deviation: {deviation} meters ({percent:.4f}%)')
    if deviation != 0.0:
        print(Back.LIGHTRED_EX + Fore.WHITE + '  FAIL  ' + Back.RESET + Fore.RESET)

print(Fore.RESET)
