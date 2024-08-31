import os

import gpxpy
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


def get_sources(gpx: GPX):
    segments = [
        [
            [[point.longitude, point.latitude] for point in segment.points]
            for segment in track.segments
        ]
        for track in gpx.tracks
    ]
    routes = [lines_feature(segment) for segment in segments]

    points = [segments[0][0][0], segments[-1][-1][-1]]
    route_points = [points_feature([point]) for point in points]
    route_start, route_end = route_points

    sources = {
        'paths': feature_collection(routes),
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
    return render_template('index.html', gpx_files=gpx_files, gpx=gpx_data)


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
            'layout': {
                'line-join': 'round',
                'line-cap': 'round'
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
            'layout': {
                'line-join': 'round',
                'line-cap': 'round'
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
    return render_template('map.html', **params)
