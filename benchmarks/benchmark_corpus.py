"""Time fastgpx over a folder of real GPX files, and compare two builds.

A single test file can hide how a change behaves on other devices, timestamp formats and file
sizes. This runs every GPX file found under the given folders (duplicates by content are skipped):

    uv run benchmarks/benchmark_corpus.py run DIR [DIR ...] -o before.json
    ... switch to the other build ...
    uv run benchmarks/benchmark_corpus.py run DIR [DIR ...] -o after.json
    uv run benchmarks/benchmark_corpus.py compare before.json after.json

`run` records which fastgpx build, Python and machine produced the numbers, and per file its path,
MD5, the time bounds and 2D/3D length of every segment, and the best-of-N
time of `load()` and of `time_bounds()`, `length_2d()` and `length_3d()`. Those three cache their
result, so each is timed on a freshly loaded document. `compare` reports files found in only one run
and files whose results or errors differ, then the total and per-file speedup over the files both
runs share. Run each side more than
once and alternate them; single runs are noisy.
"""
import argparse
import hashlib
import importlib.metadata
import json
import math
import platform
import statistics
import subprocess
import sys
import timeit
from datetime import datetime, timezone
from pathlib import Path

import fastgpx

# Files smaller than this take microseconds to process, so their ratios are mostly noise.
MIN_POINTS_FOR_SPEEDUP = 1000

# Methods on a loaded Gpx that are timed on their first, uncached call.
TIMED_METHODS = ('time_bounds', 'length_2d', 'length_3d')

# Lengths are compared with this relative tolerance, so a reordered sum does not count as a change.
LENGTH_TOLERANCE = 1e-9


def find_gpx_files(folders: list[str]) -> dict[str, Path]:
    """Map content hash to path, so the same file found in two places is timed once."""
    files: dict[str, Path] = {}
    for folder in folders:
        for path in sorted(Path(folder).rglob('*')):
            if path.suffix.lower() == '.gpx' and path.is_file():
                digest = hashlib.sha1(path.read_bytes()).hexdigest()
                files.setdefault(digest, path)
    return files


def run_info() -> dict:
    """What produced a run: the fastgpx build, Python and machine.

    fastgpx has no version or build attributes of its own, so the build is identified by the
    package version, how it was installed, and the MD5 of the extension module that was loaded.
    The repository commit is where the script ran from; for an editable install it is also what
    was built, unless the extension is stale.
    """
    dist = importlib.metadata.distribution('fastgpx')
    direct_url = dist.read_text('direct_url.json')
    extension = Path(fastgpx.__file__)
    repo = Path(__file__).resolve().parent.parent
    try:
        commit = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=repo, capture_output=True,
                                text=True, check=True).stdout.strip()
        dirty = bool(subprocess.run(['git', 'status', '--porcelain', '--untracked-files=no'],
                                    cwd=repo, capture_output=True, text=True, check=True).stdout)
    except (OSError, subprocess.CalledProcessError):
        commit, dirty = None, None
    return {
        'fastgpx_version': dist.version,
        'fastgpx_install': json.loads(direct_url) if direct_url else None,
        'fastgpx_extension': str(extension),
        'fastgpx_extension_md5': hashlib.md5(extension.read_bytes()).hexdigest(),
        'repo_commit': commit,
        'repo_dirty': dirty,
        'python': sys.version,
        'python_implementation': platform.python_implementation(),
        'platform': platform.platform(),
        'machine': platform.machine(),
        'processor': platform.processor(),
        'date': datetime.now(timezone.utc).isoformat(timespec='seconds'),
    }


def print_run_info(label: str, info: dict | None) -> None:
    if info is None:
        print(f'{label}: no run information (output from before it was recorded)')
        return
    commit = (info['repo_commit'] or '?')[:10] + (' (dirty)' if info['repo_dirty'] else '')
    print(f'{label}: fastgpx {info["fastgpx_version"]}, extension md5 '
          f'{info["fastgpx_extension_md5"][:10]}, commit {commit}, '
          f'Python {info["python"].split()[0]}, {info["platform"]} {info["machine"]}')


def read_run(path: str) -> tuple[dict | None, dict]:
    """Return (run information, rows by content hash). Older outputs are the bare rows."""
    data = json.loads(Path(path).read_text(encoding='utf-8'))
    if 'run_info' in data and 'files' in data:
        return data['run_info'], data['files']
    return None, data


def describe(bounds: fastgpx.TimeBounds) -> list[str | None]:
    return [None if t is None else t.isoformat() for t in (bounds.start_time, bounds.end_time)]


def measure(path: Path, repeats: int) -> dict:
    md5 = hashlib.md5(path.read_bytes()).hexdigest()
    try:
        gpx = fastgpx.load(path)
        segments = [s for t in gpx.tracks for s in t.segments]
        row: dict = {
            'path': str(path),
            'md5': md5,
            'points': sum(len(s.points) for s in segments),
            'segment_bounds': [describe(s.time_bounds()) for s in segments],
            'segment_lengths': [[s.length_2d(), s.length_3d()] for s in segments],
        }
    except fastgpx.Error as e:
        return {'path': str(path), 'md5': md5, 'error': f'{type(e).__name__}: {e}'}

    row['load_ms'] = min(timeit.repeat(lambda: fastgpx.load(path), number=1, repeat=repeats)) * 1e3
    for method in TIMED_METHODS:
        best = float('inf')
        for _ in range(repeats):
            call = getattr(fastgpx.load(path), method)
            start = timeit.default_timer()
            call()
            best = min(best, timeit.default_timer() - start)
        row[f'{method}_ms'] = best * 1e3
    return row


def run(args: argparse.Namespace) -> None:
    files = find_gpx_files(args.folders)
    result = {digest: measure(path, args.repeats) for digest, path in files.items()}
    output = {'run_info': run_info(), 'repeats': args.repeats, 'files': result}
    Path(args.output).write_text(json.dumps(output, indent=1), encoding='utf-8')
    errors = sum('error' in row for row in result.values())
    print(f'{len(result)} unique files, {errors} failed to load, written to {args.output}')


def compare(args: argparse.Namespace) -> None:
    before_info, before = read_run(args.before)
    after_info, after = read_run(args.after)
    print_run_info('before', before_info)
    print_run_info('after ', after_info)
    common = [d for d in before if d in after]
    for label, run_rows, other in (('before', before, after), ('after', after, before)):
        only = [d for d in run_rows if d not in other]
        if only:
            print(f'{len(only)} files only in the {label} run, left out of the comparison:')
            for digest in only:
                print(f'  {run_rows[digest]["path"]}')

    def differences(a: dict, b: dict) -> list[str]:
        if 'error' in a or 'error' in b:
            return [] if a.get('error') == b.get('error') else ['error']
        found = []
        if a['segment_bounds'] != b['segment_bounds']:
            found.append('segment_bounds')
        lengths_a = [x for pair in a['segment_lengths'] for x in pair]
        lengths_b = [x for pair in b['segment_lengths'] for x in pair]
        if len(lengths_a) != len(lengths_b) or not all(
                math.isclose(x, y, rel_tol=LENGTH_TOLERANCE) for x, y in zip(lengths_a, lengths_b)):
            found.append('segment_lengths')
        return found

    changed = {d: differences(before[d], after[d]) for d in common}
    changed = {d: keys for d, keys in changed.items() if keys}
    print(f'{len(common)} files in both runs, {len(changed)} with different results or errors')
    for digest, keys in changed.items():
        print(f'  {before[digest]["path"]}')
        for key in keys:
            print(f'    before: {before[digest].get(key)}')
            print(f'    after:  {after[digest].get(key)}')

    timed = [d for d in common if d not in changed and 'error' not in before[d]
             and before[d]['points'] >= MIN_POINTS_FOR_SPEEDUP]
    print(f'\nSpeedup over the {len(timed)} files that are in both runs, give the same '
          f'results and have at least {MIN_POINTS_FOR_SPEEDUP} points:')
    for key in ['load_ms'] + [f'{method}_ms' for method in TIMED_METHODS]:
        total_before = sum(before[d][key] for d in timed)
        total_after = sum(after[d][key] for d in timed)
        ratios = [before[d][key] / after[d][key] for d in timed if after[d][key] > 0]
        if not ratios:
            continue
        print(f'  {key:15} total {total_before:9.1f} -> {total_after:9.1f} ms '
              f'({total_before / total_after:.2f}x), per file median {statistics.median(ratios):.2f}x, '
              f'range {min(ratios):.2f}x to {max(ratios):.2f}x')


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(required=True)

    run_parser = commands.add_parser('run', help='time the installed fastgpx over GPX folders')
    run_parser.add_argument('folders', nargs='+')
    run_parser.add_argument('-o', '--output', required=True)
    run_parser.add_argument('--repeats', type=int, default=5)
    run_parser.set_defaults(func=run)

    compare_parser = commands.add_parser('compare', help='compare two run outputs')
    compare_parser.add_argument('before')
    compare_parser.add_argument('after')
    compare_parser.set_defaults(func=compare)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    main()
