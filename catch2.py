import json
import os

import gpxpy


def get_gpx_files(path: str):
    gpx_files = []
    files = os.listdir(path)
    for filename in files:
        if filename.endswith(".gpx"):
            gpx_path = os.path.join(path, filename)
            gpx_files.append(gpx_path)
    return gpx_files


GPX_PATH = 'gpx/2024 Great Roadtrip'
# GPX_PATH = 'gpx/2024 TopCamp'


def main():
    # files = get_gpx_files(GPX_PATH)
    files = ["gpx/2024 TopCamp/Connected_20240518_094959_.gpx"]
    data = []
    for gpx_filepath in files:
        fullpath = os.path.abspath(gpx_filepath)
        assert os.path.exists(fullpath)

        with open(fullpath, 'r', encoding='utf-8') as gpx_file:
            gpx = gpxpy.parse(gpx_file)

        gpx_data = {
            "path": gpx_filepath,
            "length2d": float(f'{gpx.length_2d():.4f}'),
            "length3d": float(f'{gpx.length_3d():.4f}'),
            "tracks": [],
        }

        for track in gpx.tracks:
            track_data = {
                "length2d": float(f'{track.length_2d():.4f}'),
                "length3d": float(f'{track.length_3d():.4f}'),
                "segments": [],
            }

            for segment in track.segments:
                segment_data = {
                    "length2d": float(f'{segment.length_2d():.4f}'),
                    "length3d": float(f'{segment.length_3d():.4f}'),
                }
                track_data['segments'].append(segment_data)

            gpx_data['tracks'].append(track_data)

        data.append(gpx_data)

    path = 'cpp/expected_gpx_data.json'
    with open(path, 'w', encoding='utf-8') as json_file:
        json.dump(data, json_file, indent=2)
        json_file.write('\n')


if __name__ == "__main__":
    main()
