from datetime import datetime, timedelta
from typing import Tuple
import os
import time

import gpxpy
import gpxpy.geo

import requests
from flask import Flask, abort, jsonify, render_template
from dotenv import load_dotenv

import fastgpx
from fastgpx import TimeBounds
from fastgpx import Bounds as GPXBounds
from fastgpx import Gpx as GPX

load_dotenv()

MAPBOX_ACCESS_TOKEN = os.getenv('MAPBOX_ACCESS_TOKEN')
GPX_PATH = os.getenv('GPX_PATH')

print(f'GPX path: {GPX_PATH}')

app = Flask(__name__)


def get_gpx_files(path: str):
    gpx_files = []
    files = os.listdir(path)
    for filename in files:
        if filename.endswith(".gpx"):
            gpx_files.append(filename)
    return gpx_files


def feature_collection(features: list):
    return {
        'type': 'geojson',
        'data': {
                'type': 'FeatureCollection',
                'features': features,
        }
    }


# https://datatracker.ietf.org/doc/html/rfc7946#autoid-13
def lines_feature(coordinates: list):
    return {
        'type': 'Feature',
        'geometry': {
            'type': 'MultiLineString',
            'coordinates': coordinates,
        }
    }


def points_feature(coordinates: list):
    return {
        'type': 'Feature',
        'geometry': {
            'type': 'MultiPoint',
            'coordinates': coordinates,
        }
    }


# Tuples are: Longitude, Latitude
def calculate_distance(point1: Tuple[float, float], point2: Tuple[float, float]):
    """Calculate the distance between two points in meters."""
    return fastgpx.geo.haversine_distance(
        point1[1], point1[0],
        point2[1], point2[0]
    )


def get_sources(gpx: GPX):
    # Longitude, Latitude
    segments = [
        [
            [(point.longitude, point.latitude) for point in segment.points]
            for segment in track.segments
        ]
        for track in gpx.tracks
    ]
    routes = [lines_feature(segment) for segment in segments]

    connections = []
    for track_segments in segments:
        for i in range(len(track_segments) - 1):
            last_point = track_segments[i][-1]
            first_point = track_segments[i + 1][0]

            # Calculate the distance between the last point of the current segment
            # and the first point of the next segment
            distance_between_segments = calculate_distance(last_point, first_point)

            # Only add the connection if the distance is greater than 500 meters
            if distance_between_segments > 500:
                connections.append([last_point, first_point])
    connections_points = [lines_feature([point]) for point in connections]

    print('> connections:', len(connections))

    points = [segments[0][0][0], segments[-1][-1][-1]]
    route_points = [points_feature([point]) for point in points]
    route_start, route_end = route_points

    sources = {
        'paths': feature_collection(routes),
        'connections': feature_collection(connections_points),
        'route-start': feature_collection([route_start]),
        'route-end': feature_collection([route_end]),
    }
    return sources


@app.route("/all")
def all():
    assert GPX_PATH is not None
    gpx_files = get_gpx_files(GPX_PATH)

    gpx_data = {
        'filename': 'all',
        'name': 'Multiple files',
        'distance': 0.0,
        'time': TimeBounds(None, None),
        'duration': timedelta(),
    }
    start_time = None
    end_time = None

    for gpx_filename in gpx_files:
        filepath = os.path.join(GPX_PATH, gpx_filename)
        if os.path.exists(filepath):

            print(f'Processing {filepath} ...')
            gpx = fastgpx.parse(filepath)

            print(f'> length_3d()')
            distance_meters = gpx.length_3d()

            print(f'> get_time_bounds()')
            time = gpx.get_time_bounds()
            assert isinstance(gpx_data['time'], TimeBounds)
            if time.start_time and time.end_time:
                duration = time.end_time - time.start_time
                gpx_data['duration'] += duration

                if start_time is None:
                    start_time = time.start_time
                else:
                    start_time = min(time.start_time, start_time)

                if end_time is None:
                    end_time = time.end_time
                else:
                    end_time = max(time.end_time, end_time)

            gpx_data['distance'] += distance_meters

    gpx_data['time'] = TimeBounds(start_time, end_time)
    gpx_data['distance'] = f"{(gpx_data['distance'] / 1000):.2f}"
    return render_template('index.html.j2', gpx_files=gpx_files, gpx=gpx_data)


@app.route("/")
@app.route("/<gpx_filename>")
def index(gpx_filename=None):
    assert GPX_PATH is not None
    gpx_files = get_gpx_files(GPX_PATH)

    gpx_data = {}
    if gpx_filename:
        filepath = os.path.join(GPX_PATH, gpx_filename)
        if os.path.exists(filepath):

            with open(filepath, 'r', encoding='utf-8') as gpx_file:
                gpx = gpxpy.parse(gpx_file)

            distance_meters = gpx.length_3d()
            distance_km = distance_meters / 1000

            time = gpx.get_time_bounds()
            if time.start_time and time.end_time:
                duration = time.end_time - time.start_time
            else:
                duration = 'Unknown'

            gpx_data = {
                'filename': gpx_filename,
                'name': gpx.name,
                'distance': f"{distance_km:.2f}",
                'time': time,
                'duration': duration,
            }
    return render_template('index.html.j2', gpx_files=gpx_files, gpx=gpx_data)


@app.route("/map/<gpx>")
def map(gpx=None):
    assert GPX_PATH is not None
    assert gpx is not None

    if gpx == 'all':
        gpx_files = get_gpx_files(GPX_PATH)
    else:
        filepath = os.path.join(GPX_PATH, gpx)
        if not os.path.exists(filepath):
            abort(404)
        gpx_files = [gpx]

    start_time = time.perf_counter()

    map_bounds = None
    map_sources = dict()
    for gpx_filename in gpx_files:
        filepath = os.path.join(GPX_PATH, gpx_filename)

        print(f'{filepath} ...')
        gpx = fastgpx.parse(filepath)

        bounds = gpx.get_bounds()
        assert bounds is not None

        if map_bounds is None:
            map_bounds = bounds
        else:
            map_bounds = map_bounds.max_bounds(bounds=bounds)

        sources = get_sources(gpx)
        # TODO: Ideally create a connection line if the start of a new GPX is
        #   far enough away from the end of the last one.
        #   Example: Ending a day on a ferry and starting the next day on the
        #   other end. The ride after the ferry was too short to be recorded.
        for key, value in sources.items():
            if key not in map_sources:
                print('> assign sources')
                map_sources[key] = value
            else:
                print('> merge sources')
                map_sources[key]['data']['features'].extend(value['data']['features'])

    end_time = time.perf_counter()

    execution_time = end_time - start_time
    print(f"Execution time: {execution_time} seconds")

    layers = [
        {
            'id': 'route',
            'type': 'line',
            'source': 'paths',
            'paint': {
                'line-color': '#b33',
                'line-width': 4,
            },
            'layout': {
                'line-join': 'round',
                'line-cap': 'round'
            },
        },
        {
            'id': 'betweens',
            'type': 'line',
            'source': 'connections',
            'paint': {
                'line-color': '#b33',
                'line-width': 2,
                'line-dasharray': [3, 3],
            },
            # 'filter': ['==', '$type', 'LineString']
        },
        {
            'id': 'routeStart',
            'type': 'circle',
            'source': 'route-start',
            'paint': {
                'circle-radius': 6,
                'circle-color': '#22B422'
            },
            # 'filter': ['==', '$type', 'Point']
        },
        {
            'id': 'routeEnd',
            'type': 'circle',
            'source': 'route-end',
            'paint': {
                'circle-radius': 6,
                'circle-color': '#B42222'
            },
            # 'filter': ['==', '$type', 'Point']
        },
    ]

    if len(gpx_files) > 1:
        # Kludge to not display the green start points.
        # del layers[2]
        # layers[2]['paint']['circle-color'] = '#B42222'

        # Don't display start/end points.
        layers.pop()
        layers.pop()

    marker_scale = 0.6
    markers = []
    if len(gpx_files) == 1:
        for layer in layers:
            if layer['type'] != 'circle':
                continue
            layer_source = layer['source']
            layer_color = layer['paint']['circle-color']
            source = map_sources[layer_source]
            for feature in source['data']['features']:
                if feature['geometry']['type'] == 'MultiPoint':
                    for position in feature['geometry']['coordinates']:
                        marker = {
                            'options': {
                                'color': layer_color,
                                'scale': marker_scale,
                            },
                            'position': position,
                        }
                        markers.append(marker)

    assert map_bounds is not None
    bbox = f'[{map_bounds.min_longitude},{map_bounds.min_latitude},{map_bounds.max_longitude},{map_bounds.max_latitude}]'

    padding = 50 if len(gpx_files) == 1 else 10

    params = {
        'mapbox_access_token': MAPBOX_ACCESS_TOKEN,
        'bounds': bbox,
        'sources': map_sources,
        'layers': layers,
        'markers': markers,
        'padding': padding,
    }
    return render_template('map.html.j2', **params)
