import os

import gpxpy
import gpxpy.geo
from gpxpy.gpx import GPX

import requests
from flask import Flask, abort, jsonify, render_template
from dotenv import load_dotenv

load_dotenv()

MAPBOX_ACCESS_TOKEN = os.getenv('MAPBOX_ACCESS_TOKEN')
GPX_PATH = os.getenv('GPX_PATH')

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


def calculate_distance(point1, point2):
    """Calculate the distance between two points in meters."""
    return gpxpy.geo.haversine_distance(
        point1[1], point1[0],
        point2[1], point2[0]
    )


def get_sources(gpx: GPX):
    segments = [
        [
            [[point.longitude, point.latitude] for point in segment.points]
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

    print('connections:', len(connections))

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

            gpx_data = {
                'filename': gpx_filename,
                'name': gpx.name,
                'distance': f"{distance_km:.2f}",
            }
    return render_template('index.html.j2', gpx_files=gpx_files, gpx=gpx_data)


@app.route("/map/<gpx>")
def map(gpx=None):
    assert GPX_PATH is not None
    assert gpx is not None

    filepath = os.path.join(GPX_PATH, gpx)
    if not os.path.exists(filepath):
        abort(404)

    with open(filepath, 'r', encoding='utf-8') as gpx_file:
        gpx = gpxpy.parse(gpx_file)

    bounds = gpx.get_bounds()
    assert bounds is not None
    bbox = f'[{bounds.min_longitude},{bounds.min_latitude},{bounds.max_longitude},{bounds.max_latitude}]'

    sources = get_sources(gpx)
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
    stats = {}

    params = {
        'mapbox_access_token': MAPBOX_ACCESS_TOKEN,
        'bounds': bbox,
        'stats': stats,
        'sources': sources,
        'layers': layers,
    }
    return render_template('map.html.j2', **params)
