import os

import gpxpy
import requests
from flask import Flask, abort, render_template
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


def get_gpx_polyline(gpx):
    coords = []
    for tracks in gpx.tracks:
        for segments in tracks.segments:
            for point in segments.points:
                # long, lat
                coords.append(f'[{point.longitude},{point.latitude}]')
    coords_js = ",\n              ".join(coords)
    return coords_js


@app.route("/")
@app.route("/<gpx>")
def index(gpx=None):
    assert GPX_PATH is not None
    gpx_files = get_gpx_files(GPX_PATH)
    return render_template('index.html', gpx_files=gpx_files, gpx=gpx)


@app.route("/map/<gpx>")
def map(gpx=None):
    assert GPX_PATH is not None
    assert gpx is not None

    filepath = os.path.join(GPX_PATH, gpx)
    if not os.path.exists(filepath):
        abort(404)

    with open(filepath, 'r') as gpx_file:
        gpx = gpxpy.parse(gpx_file)

    bounds = gpx.get_bounds()
    assert bounds is not None
    bbox = f'[{bounds.min_longitude},{bounds.min_latitude},{bounds.max_longitude},{bounds.max_latitude}]'

    coords = get_gpx_polyline(gpx)

    params = {
        'bounds': bbox,
        'coords': coords,
        'mapbox_access_token': MAPBOX_ACCESS_TOKEN,
    }
    return render_template('map.html', **params)
