import json
import os
from pathlib import Path

import fastgpx


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
    # files = ["gpx/2024 TopCamp/Connected_20240518_094959_.gpx"]

    files = []
    files.extend(get_gpx_files('gpx/2024 Great Roadtrip'))
    files.extend(get_gpx_files('gpx/2024 Sommertur'))
    files.extend(get_gpx_files('gpx/2024 TopCamp'))

    data = []
    for gpx_filepath in files:
        gpx_filepath = Path(gpx_filepath).as_posix()

        print(gpx_filepath)
        fullpath = os.path.abspath(gpx_filepath)
        assert os.path.exists(fullpath)

        gpx = fastgpx.parse(fullpath)

        gpx_data = {
            "path": gpx_filepath,
            "length2d": float(f'{gpx.length_2d():.4f}'),
            "length3d": float(f'{gpx.length_3d():.4f}'),
            "tracks": [],
        }
        gpx_time = gpx.time_bounds()
        if gpx_time.start_time is not None and gpx_time.end_time is not None:
            gpx_data["time_bounds"] = {
                "start_time": int(gpx_time.start_time.timestamp()),
                "end_time": int(gpx_time.end_time.timestamp()),
            }

        for track in gpx.tracks:
            track_data = {
                "length2d": float(f'{track.length_2d():.4f}'),
                "length3d": float(f'{track.length_3d():.4f}'),
                "segments": [],
            }
            track_time = track.time_bounds()
            if track_time.start_time is not None and track_time.end_time is not None:
                track_data["time_bounds"] = {
                    "start_time": int(track_time.start_time.timestamp()),
                    "end_time": int(track_time.end_time.timestamp()),
                }

            for segment in track.segments:
                segment_data = {
                    "length2d": float(f'{segment.length_2d():.4f}'),
                    "length3d": float(f'{segment.length_3d():.4f}'),
                }
                segment_time = segment.time_bounds()
                if segment_time.start_time is not None and segment_time.end_time is not None:
                    segment_data["time_bounds"] = {
                        "start_time": int(segment_time.start_time.timestamp()),
                        "end_time": int(segment_time.end_time.timestamp()),
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
