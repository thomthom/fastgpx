import os

import polyline
from colorama import Fore, Back

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


def benchmark_fastgpx():
    polylines = []
    for gpx_filepath in gpx_files:
        gpx = fastgpx.parse(gpx_filepath)
        for track in gpx.tracks:
            for segment in track.segments:
                points = segment.points
                encoded = fastgpx.polyline.encode(points, precision=6)
                polylines.append(encoded)
    return polylines


def benchmark_polyline():
    polylines = []
    for gpx_filepath in gpx_files:
        gpx = fastgpx.parse(gpx_filepath)
        for track in gpx.tracks:
            for segment in track.segments:
                # Note: Extra overhead because this type conversion is needed.
                points = [(point.latitude, point.longitude) for point in segment.points]
                encoded = polyline.encode(points, precision=6)
                polylines.append(encoded)
    return polylines


if __name__ == '__main__':
    import timeit

    iterations = 10

    gpx_files = get_gpx_files(GPX_PATH)
    print(Fore.LIGHTBLUE_EX + f'GPX path: {GPX_PATH}')
    print(Fore.LIGHTBLUE_EX + f'GPX files: {len(gpx_files)}')
    print(Fore.LIGHTBLUE_EX + f'Iterations: {iterations}')
    # print('\n'.join(gpx_files))

    print()
    print(Fore.LIGHTYELLOW_EX + 'benchmarking fastgpx.polyline.encode' + Fore.RESET)
    print(timeit.timeit("benchmark_fastgpx()", globals=locals(), number=iterations))

    print()
    print(Fore.LIGHTYELLOW_EX + 'benchmarking polyline.encode' + Fore.RESET)
    print(timeit.timeit("benchmark_polyline()", globals=locals(), number=iterations))

    print(Fore.RESET)
