import os
from dataclasses import dataclass

import mapbox
import gpxpy
import gpxpy.gpx
import polyline
import requests
from dotenv import load_dotenv
from PIL import Image, ImageDraw

load_dotenv()

MAPBOX_ACCESS_TOKEN = os.getenv('MAPBOX_ACCESS_TOKEN')

STATIC_IMAGES = mapbox.Static(access_token=MAPBOX_ACCESS_TOKEN)


@dataclass
class Size2d:
    width: int
    height: int


def process_gpx_file(filepath):
    with open(filepath, 'r') as gpx_file:
        gpx = gpxpy.parse(gpx_file)

    size = Size2d(width=300, height=200)
    bounds = gpx.get_bounds()
    mapbox_imagepath = 'mapbox.png'
    # map_image_from_bounds(bounds=bounds, size=size,
    #                       output_path=mapbox_imagepath)
    image_path = 'map.png'
    draw_tracks(gpx, mapbox_imagepath, image_path)


def map_image_from_bounds(bounds: gpxpy.gpx.GPXBounds, size: Size2d, output_path: str):
    # https://api.mapbox.com/styles/v1/{username}/{style_id}/static/{overlay}/{lon},{lat},{zoom},{bearing},{pitch}|{bbox}|{auto}/{width}x{height}{@2x}
    # mapbox://styles/onionarmy/cm0hxr4fk004g01pma59r1mqv
    username = 'onionarmy'
    style_id = 'cm0hxr4fk004g01pma59r1mqv'
    width = size.width
    height = size.height
    bbox = f'[{bounds.min_longitude},{bounds.min_latitude},{bounds.max_longitude},{bounds.max_latitude}]'
    url = f'https://api.mapbox.com/styles/v1/{username}/{style_id}/static/{bbox}/{width}x{height}'

    params = {
        'access_token': MAPBOX_ACCESS_TOKEN,
    }

    response = requests.get(url=url, params=params)

    if response.status_code == 200:
        with open(output_path, "wb") as output:
            output.write(response.content)
        print(f"Static map image saved as '{output_path}'")
    else:
        print(f"Error: {response.status_code} - {response.text}")


def pixel_segment(bounds: gpxpy.gpx.GPXBounds, image: Image, segment: gpxpy.gpx.GPXTrackSegment):
    width, height = image.size
    ratio = float(width) / float(height)

    min_lon = bounds.min_longitude
    max_lon = bounds.max_longitude
    min_lat = bounds.min_latitude
    max_lat = bounds.max_latitude

    world_width = max_lon - min_lon
    world_height = max_lat - min_lat

    scale_w = float(world_width) / float(width)
    scale_h = float(world_height) / float(height)
    scale_h = scale_w * ratio

    # scale_w = float(width) / float(world_width)
    # scale_h = float(height) / float(world_height)

    print()
    print('image', (width, height))
    print('world', (world_width, world_height))
    print('scale', (scale_w, scale_h))
    print('segment', len(segment.points))

    pixels = []
    for point in segment.points:
        lon = point.longitude
        lat = point.latitude

        # x = (lon - min_lon) / (max_lon - min_lon) * width
        # y = height - (lat - min_lat) / (max_lat - min_lat) * height

        x = (lon * scale_w) * width
        y = (lat * scale_h) * height

        coord = (x, y)
        # print(coord)
        pixels.append(coord)
    print('pixel ( 0)', pixels[0])
    print('pixel (-1)', pixels[-1])
    return pixels


def draw_tracks(gpx: gpxpy.gpx.GPX, input_imagepath: str, output_imagepath: str):
    image = Image.open(input_imagepath)
    draw = ImageDraw.Draw(image)
    bounds = gpx.get_bounds()
    for track in gpx.tracks:
        for segment in track.segments:
            pixel_points = pixel_segment(bounds, image, segment)
            draw.line(pixel_points, fill="red", width=2)
    image.save(output_imagepath)
    # image.show()


def main():
    directory = "gpx/2024 Great Roadtrip"
    for filename in os.listdir(directory):
        if filename.endswith(".gpx"):
            filepath = os.path.join(directory, filename)
            process_gpx_file(filepath=filepath)
            break


main()
