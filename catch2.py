import json
from pathlib import Path

import fastgpx


def get_gpx_files(dir_path: Path):
    return sorted(dir_path.glob("*.gpx"))


GPX_ROOTS = [
    Path("gpx/2024 Great Roadtrip"),
    Path("gpx/2024 Sommertur"),
    Path("gpx/2024 TopCamp"),
]


def main():
    files = []
    for root in GPX_ROOTS:
        files.extend(get_gpx_files(root))

    data = []
    for gpx_path in files:
        print(gpx_path.as_posix())

        assert gpx_path.exists()

        gpx = fastgpx.parse(str(gpx_path.resolve()))

        gpx_data = {
            "path": gpx_path.as_posix(),
            "length2d": round(gpx.length_2d(), 4),
            "length3d": round(gpx.length_3d(), 4),
            "tracks": [],
        }

        gpx_time = gpx.time_bounds()
        if gpx_time.start_time and gpx_time.end_time:
            gpx_data["time_bounds"] = {
                "start_time": int(gpx_time.start_time.timestamp()),
                "end_time": int(gpx_time.end_time.timestamp()),
            }

        for track in gpx.tracks:
            track_data = {
                "length2d": round(track.length_2d(), 4),
                "length3d": round(track.length_3d(), 4),
                "segments": [],
            }

            track_time = track.time_bounds()
            if track_time.start_time and track_time.end_time:
                track_data["time_bounds"] = {
                    "start_time": int(track_time.start_time.timestamp()),
                    "end_time": int(track_time.end_time.timestamp()),
                }

            for segment in track.segments:
                segment_data = {
                    "length2d": round(segment.length_2d(), 4),
                    "length3d": round(segment.length_3d(), 4),
                }

                segment_time = segment.time_bounds()
                if segment_time.start_time and segment_time.end_time:
                    segment_data["time_bounds"] = {  # type: ignore
                        "start_time": int(segment_time.start_time.timestamp()),
                        "end_time": int(segment_time.end_time.timestamp()),
                    }

                track_data["segments"].append(segment_data)

            gpx_data["tracks"].append(track_data)

        data.append(gpx_data)

    output_path = Path("cpp/expected_gpx_data.json")
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with output_path.open("w", encoding="utf-8") as json_file:
        json.dump(data, json_file, indent=2)
        json_file.write("\n")


if __name__ == "__main__":
    main()
