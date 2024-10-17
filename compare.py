import math
import os

import gpxpy
from colorama import Fore, Back, Style

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


def compare(path):
    files = get_gpx_files(path)
    for gpx_filepath in files:
        print()
        print(Fore.LIGHTCYAN_EX + gpx_filepath + Fore.RESET)

        fullpath = os.path.abspath(gpx_filepath)
        print(Fore.LIGHTBLACK_EX + fullpath + Fore.RESET)

        assert os.path.exists(fullpath)

        actual_gpx = fastgpx.parse(fullpath)

        with open(fullpath, 'r', encoding='utf-8') as gpx_file:
            expected_gpx = gpxpy.parse(gpx_file)
        expected = expected_gpx.length_2d()

        actual_gpx = fastgpx.parse(fullpath)
        actual = actual_gpx.length_2d()

        print(Fore.LIGHTBLACK_EX + f'Expected: {expected}' + Fore.RESET)
        print(Fore.LIGHTBLACK_EX + f'  Actual: {actual}' + Fore.RESET)

        if math.isclose(expected, actual, abs_tol=1e-04):
            print(Back.LIGHTGREEN_EX + Fore.WHITE + f'  PASS  ' + Back.RESET)
        else:
            print(Back.LIGHTRED_EX + Fore.WHITE + f'  FAIL  ' + Back.RESET)


def main():
    try:
        compare(GPX_PATH)
    finally:
        print(Back.RESET + Fore.RESET)


if __name__ == "__main__":
    main()
