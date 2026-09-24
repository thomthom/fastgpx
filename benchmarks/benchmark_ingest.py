"""Time the fastgpx side of Sleipnir's GPX upload, step by step, and compare two builds.

Sleipnir stores an uploaded file with `create_gpx_file` (`apps/maps/gpx_ingest.py`) after the
upload view has decoded and parsed it (`apps/maps/views/api/user_gpx.py`). This repeats the
fastgpx calls of that path, in the same order, without Django: the `LineString`, `Polygon` and
database work are left out, but the Python objects handed to them are still built.

    uv run benchmarks/benchmark_ingest.py run DIR [DIR ...] -o before.json
    ... switch to the other build ...
    uv run benchmarks/benchmark_ingest.py run DIR [DIR ...] -o after.json
    uv run benchmarks/benchmark_ingest.py compare before.json after.json

Two variants are timed, each on its own fresh parse:

- `current`: what Sleipnir does today, copying every point into a new `fastgpx.LatLong` and
  building the `(lon, lat)` tuples from the copies.
- `no_copy`: the path after thomthom/sleipnir#596, building the tuples from the parsed points.

Each file's bytes are read once up front, as the upload view has them in memory. Every step is
timed per file on each round; a file's best and median over the rounds are kept, and the summary
sums them over the files and divides by the track points. The garbage collector stays on, as it
is in production. Run each side more than once and alternate them; single runs are noisy.
"""
import argparse
import hashlib
import json
import statistics
import sys
from pathlib import Path
from time import perf_counter_ns

import fastgpx

sys.path.insert(0, str(Path(__file__).resolve().parent))
from benchmark_corpus import find_gpx_files, print_run_info, read_run, run_info  # noqa: E402

VARIANTS = {
    'current': ('decode', 'parse', 'track_time_bounds', 'list_points', 'latlong_copy',
                'coords_from_copy', 'segment_summary', 'free_lists', 'merge_bounds'),
    'no_copy': ('decode', 'parse', 'track_time_bounds', 'list_points', 'coords_from_points',
                'segment_summary', 'free_lists', 'merge_bounds'),
}

STEP_LABELS = {
    'decode': 'content.decode("utf-8")',
    'parse': 'fastgpx.parse(text)',
    'track_time_bounds': 'track.time_bounds() and its start/end',
    'list_points': 'list(segment.points)',
    'latlong_copy': 'a new LatLong per point',
    'coords_from_copy': '(lon, lat) tuples from the copies',
    'coords_from_points': '(lon, lat) tuples from the points',
    'segment_summary': 'segment bounds, length_2d, time bounds',
    'free_lists': 'freeing the point lists',
    'merge_bounds': 'merge bounds per track and file',
}


def extract_time_bounds(time_bounds: fastgpx.TimeBounds | None) -> tuple:
    """Sleipnir's `apps.maps.gpx.extract_time_bounds`."""
    if time_bounds is None or time_bounds.is_empty():
        return None, None
    return time_bounds.start_time, time_bounds.end_time


def bounds_corners(bounds: fastgpx.Bounds | None) -> tuple | None:
    """The attribute reads of Sleipnir's `bounds_to_polygon`, without the Polygon."""
    if bounds is None or bounds.is_empty():
        return None
    return (bounds.min_longitude, bounds.min_latitude, bounds.max_longitude, bounds.max_latitude)


def merge_bounds(bounds_list: list) -> tuple | None:
    """Sleipnir's `apps.maps.gpx.merge_bounds`, without the Polygon."""
    valid = [b for b in bounds_list if b is not None and not b.is_empty()]
    if not valid:
        return None
    merged = fastgpx.Bounds()
    for b in valid:
        merged.add(b)
    return bounds_corners(merged)


def ingest(content: bytes, copy: bool) -> dict[str, int]:
    """One pass of the upload path over `content`; returns nanoseconds per step."""
    times = dict.fromkeys(VARIANTS['current' if copy else 'no_copy'], 0)

    t0 = perf_counter_ns()
    text = content.decode('utf-8')
    t1 = perf_counter_ns()
    gpx = fastgpx.parse(text)
    t2 = perf_counter_ns()
    times['decode'] += t1 - t0
    times['parse'] += t2 - t1

    all_bounds = []
    for track in gpx.tracks:
        t0 = perf_counter_ns()
        extract_time_bounds(track.time_bounds())
        times['track_time_bounds'] += perf_counter_ns() - t0

        track_bounds = []
        for segment in track.segments:
            t0 = perf_counter_ns()
            segment_points = list(segment.points)
            t1 = perf_counter_ns()
            times['list_points'] += t1 - t0
            if copy:
                latlongs = [fastgpx.LatLong(latitude=p.latitude, longitude=p.longitude)
                            for p in segment_points]
                t2 = perf_counter_ns()
                # latlong_list_to_linestring
                coords = [(p.longitude, p.latitude) for p in latlongs] if len(latlongs) >= 2 else None
                t3 = perf_counter_ns()
                times['latlong_copy'] += t2 - t1
                times['coords_from_copy'] += t3 - t2
            else:
                coords = ([(p.longitude, p.latitude) for p in segment_points]
                          if len(segment_points) >= 2 else None)
                t3 = perf_counter_ns()
                times['coords_from_points'] += t3 - t1
            del coords

            t0 = perf_counter_ns()
            segment_bounds = segment.bounds()
            segment.length_2d()
            extract_time_bounds(segment.time_bounds())
            bounds_corners(segment_bounds)
            track_bounds.append(segment_bounds)
            times['segment_summary'] += perf_counter_ns() - t0

            # Free the point lists here, so their clean-up is timed on its own instead of
            # landing in the next segment's steps (or, after the last segment, not at all).
            t0 = perf_counter_ns()
            del segment_points
            if copy:
                del latlongs
            times['free_lists'] += perf_counter_ns() - t0

        t0 = perf_counter_ns()
        merge_bounds(track_bounds)
        all_bounds.extend(track_bounds)
        times['merge_bounds'] += perf_counter_ns() - t0

    t0 = perf_counter_ns()
    merge_bounds(all_bounds)
    times['merge_bounds'] += perf_counter_ns() - t0
    return times


def count_points(content: bytes) -> int:
    gpx = fastgpx.parse(content.decode('utf-8'))
    return sum(len(s.points) for t in gpx.tracks for s in t.segments)


def run(args: argparse.Namespace) -> None:
    files = find_gpx_files(args.folders)
    rows: dict[str, dict] = {}
    contents: dict[str, bytes] = {}
    for digest, path in files.items():
        content = path.read_bytes()
        row = rows[digest] = {'path': str(path), 'md5': hashlib.md5(content).hexdigest()}
        try:
            row['points'] = count_points(content)
        except (fastgpx.Error, UnicodeDecodeError) as e:
            row['error'] = f'{type(e).__name__}: {e}'
            continue
        contents[digest] = content

    samples = {d: {v: {s: [] for s in steps} for v, steps in VARIANTS.items()} for d in contents}
    for round_ in range(args.rounds):
        for digest, content in contents.items():
            # Alternate which variant goes first, so neither always runs on the other's warm caches.
            order = list(VARIANTS) if round_ % 2 == 0 else list(reversed(VARIANTS))
            for variant in order:
                for step, ns in ingest(content, copy=variant == 'current').items():
                    samples[digest][variant][step].append(ns)
        print(f'round {round_ + 1} of {args.rounds} done', file=sys.stderr)

    for digest, by_variant in samples.items():
        rows[digest]['steps'] = {
            variant: {step: {'best_ns': min(ns), 'median_ns': statistics.median(ns)}
                      for step, ns in by_step.items()}
            for variant, by_step in by_variant.items()}

    output = {'run_info': run_info(), 'rounds': args.rounds, 'files': rows}
    Path(args.output).write_text(json.dumps(output, indent=1), encoding='utf-8')
    errors = sum('error' in row for row in rows.values())
    print(f'{len(rows)} unique files, {errors} failed to parse, written to {args.output}\n')
    print_summary(rows, list(contents))


def per_point(rows: dict, digests: list[str], variant: str, step: str, stat: str) -> float:
    points = sum(rows[d]['points'] for d in digests)
    return sum(rows[d]['steps'][variant][step][stat] for d in digests) / points if points else 0.0


def print_summary(rows: dict, digests: list[str]) -> None:
    points = sum(rows[d]['points'] for d in digests)
    print(f'ns per track point over {len(digests)} files, {points} points (best / median):')
    for variant, steps in VARIANTS.items():
        print(f'  {variant}')
        totals = [0.0, 0.0]
        for step in steps:
            best = per_point(rows, digests, variant, step, 'best_ns')
            median = per_point(rows, digests, variant, step, 'median_ns')
            totals[0] += best
            totals[1] += median
            print(f'    {STEP_LABELS[step]:42} {best:7.1f} / {median:7.1f}')
        print(f'    {"total":42} {totals[0]:7.1f} / {totals[1]:7.1f}')


def compare(args: argparse.Namespace) -> None:
    before_info, before = read_run(args.before)
    after_info, after = read_run(args.after)
    print_run_info('before', before_info)
    print_run_info('after ', after_info)
    for label, run_rows, other in (('before', before, after), ('after', after, before)):
        only = [d for d in run_rows if d not in other]
        if only:
            print(f'{len(only)} files only in the {label} run, left out of the comparison:')
            for digest in only:
                print(f'  {run_rows[digest]["path"]}')
    common = [d for d in before if d in after and 'error' not in before[d] and 'error' not in after[d]]
    failed = [d for d in before if d in after and d not in common]
    if failed:
        print(f'{len(failed)} files failed to parse in one or both runs, left out:')
        for digest in failed:
            print(f'  {before[digest]["path"]}: {before[digest].get("error")} / {after[digest].get("error")}')

    points = sum(before[d]['points'] for d in common)
    print(f'\nns per track point over the {len(common)} files in both runs, {points} points '
          f'(sum of per-file best, before -> after):')
    for variant, steps in VARIANTS.items():
        print(f'  {variant}')
        total_before = total_after = 0.0
        for step in steps:
            b = per_point(before, common, variant, step, 'best_ns')
            a = per_point(after, common, variant, step, 'best_ns')
            total_before += b
            total_after += a
            print(f'    {STEP_LABELS[step]:42} {b:7.1f} -> {a:7.1f}  ({b / a if a else 0:.2f}x)')
        print(f'    {"total":42} {total_before:7.1f} -> {total_after:7.1f}  '
              f'({total_before / total_after if total_after else 0:.2f}x)')


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(required=True)

    run_parser = commands.add_parser('run', help="time Sleipnir's upload path over GPX folders")
    run_parser.add_argument('folders', nargs='+')
    run_parser.add_argument('-o', '--output', required=True)
    run_parser.add_argument('--rounds', type=int, default=5)
    run_parser.set_defaults(func=run)

    compare_parser = commands.add_parser('compare', help='compare two run outputs')
    compare_parser.add_argument('before')
    compare_parser.add_argument('after')
    compare_parser.set_defaults(func=compare)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    main()
