"""Record, and check, which GPX files the benchmark corpora hold.

The real-world benchmark folders are not in the repository (they are large, and the Sleipnir
uploads are private), so benchmark results cannot be traced back to their files unless the files
are listed somewhere that is. `corpus_manifest.json` next to this script is that list: per file
its name, MD5, size, track-point count and how many of those points carry a time, plus where the
folder came from and when.

    uv run benchmarks/corpus_manifest.py verify
    uv run benchmarks/corpus_manifest.py verify --full
    uv run benchmarks/corpus_manifest.py generate [--source gpx/sleipnir=<upload folder>]

`verify` checks the local folders against the manifest by MD5 and exits non-zero on any missing,
extra or changed file; `--full` also recounts the points with the installed fastgpx. `generate`
rewrites the manifest from the local folders. Where a file came from is kept from the existing
manifest, or recomputed by MD5 from the folder given with `--source`.
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import fastgpx

REPO = Path(__file__).resolve().parent.parent
MANIFEST = Path(__file__).resolve().parent / 'corpus_manifest.json'

# Folder (relative to the repository) -> where it came from and when. Update these when a folder
# is refreshed, then run `generate`.
CORPORA = {
    'gpx/sleipnir': {
        'source': "Sleipnir's local upload folder, sleipnir/uploaded/gpx/ (2026/02 and 2026/08): "
                  '145 files, of which 106 are unique by content. One copy of each is kept, '
                  'under its upload file name.',
        'copied': '2026-09-24',
    },
    'gpx/TET': {
        'source': 'Trans Euro Trail country routes from transeurotrail.org, one file per country. '
                  'The original download date was not recorded; the date is that of this copy.',
        'copied': '2026-09-24',
    },
}


def md5_of(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def gpx_files(folder: Path) -> list[Path]:
    return sorted(p for p in folder.rglob('*') if p.is_file() and p.suffix.lower() == '.gpx')


def count_points(path: Path) -> tuple[int, int]:
    """(track points, track points with a time) as the installed fastgpx reads them."""
    gpx = fastgpx.load(path)
    points = timed = 0
    for track in gpx.tracks:
        for segment in track.segments:
            for point in segment.points:
                points += 1
                timed += point.time is not None
    return points, timed


def source_paths(source: Path) -> dict[str, list[str]]:
    """MD5 -> every path under `source` with that content, relative to `source`."""
    found: dict[str, list[str]] = {}
    for path in gpx_files(source):
        found.setdefault(md5_of(path), []).append(path.relative_to(source).as_posix())
    return found


def describe_folder(folder: str, previous: dict | None, source: Path | None) -> dict:
    root = REPO / folder
    if not root.is_dir():
        sys.exit(f'{root} does not exist')
    known_sources = {f['md5']: f.get('source_paths') for f in (previous or {}).get('files', [])}
    from_source = source_paths(source) if source else None
    files = []
    for path in gpx_files(root):
        md5 = md5_of(path)
        points, timed = count_points(path)
        entry = {
            'name': path.relative_to(root).as_posix(),
            'md5': md5,
            'size': path.stat().st_size,
            'points': points,
            'timed_points': timed,
        }
        paths = from_source.get(md5) if from_source is not None else known_sources.get(md5)
        if paths:
            entry['source_paths'] = paths
        files.append(entry)
    return {
        'folder': folder,
        **CORPORA[folder],
        'totals': {
            'files': len(files),
            'size': sum(f['size'] for f in files),
            'points': sum(f['points'] for f in files),
            'timed_points': sum(f['timed_points'] for f in files),
        },
        'files': files,
    }


def load_manifest() -> dict:
    if not MANIFEST.is_file():
        return {'corpora': []}
    return json.loads(MANIFEST.read_text(encoding='utf-8'))


def generate(args: argparse.Namespace) -> None:
    sources = {}
    for item in args.source:
        folder, _, path = item.partition('=')
        if folder not in CORPORA or not path:
            sys.exit(f'--source takes FOLDER=PATH with FOLDER one of {", ".join(CORPORA)}')
        sources[folder] = Path(path)
    previous = {c['folder']: c for c in load_manifest()['corpora']}
    corpora = [describe_folder(folder, previous.get(folder), sources.get(folder))
               for folder in CORPORA]
    manifest = {
        'about': 'GPX files in the local benchmark folders. Written by benchmarks/corpus_manifest.py.',
        'corpora': corpora,
    }
    MANIFEST.write_text(json.dumps(manifest, indent=1) + '\n', encoding='utf-8')
    for corpus in corpora:
        totals = corpus['totals']
        print(f'{corpus["folder"]}: {totals["files"]} files, {totals["size"] / 1e6:.1f} MB, '
              f'{totals["points"]} points, {totals["timed_points"]} with a time')
    print(f'written to {MANIFEST}')


def verify(args: argparse.Namespace) -> None:
    problems = 0
    for corpus in load_manifest()['corpora']:
        root = REPO / corpus['folder']
        expected = {f['name']: f for f in corpus['files']}
        actual = {p.relative_to(root).as_posix(): p for p in gpx_files(root)} if root.is_dir() else {}
        missing = sorted(set(expected) - set(actual))
        extra = sorted(set(actual) - set(expected))
        changed = []
        for name in sorted(set(expected) & set(actual)):
            entry, path = expected[name], actual[name]
            if md5_of(path) != entry['md5']:
                changed.append(f'{name}: content differs')
            elif args.full and count_points(path) != (entry['points'], entry['timed_points']):
                changed.append(f'{name}: fastgpx now reads {count_points(path)} (points, timed), '
                               f'manifest has ({entry["points"]}, {entry["timed_points"]})')
        status = 'ok' if not (missing or extra or changed) else 'MISMATCH'
        print(f'{corpus["folder"]}: {len(expected)} files in manifest, {len(actual)} on disk, {status}')
        for label, names in (('missing', missing), ('not in manifest', extra), ('changed', changed)):
            for name in names:
                print(f'  {label}: {name}')
        problems += len(missing) + len(extra) + len(changed)
    if problems:
        sys.exit(1)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(required=True)

    generate_parser = commands.add_parser('generate', help='rewrite the manifest from the folders')
    generate_parser.add_argument('--source', action='append', default=[], metavar='FOLDER=PATH',
                                 help='where a folder was copied from, to record source paths')
    generate_parser.set_defaults(func=generate)

    verify_parser = commands.add_parser('verify', help='check the folders against the manifest')
    verify_parser.add_argument('--full', action='store_true',
                               help='also recount the points with the installed fastgpx')
    verify_parser.set_defaults(func=verify)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    main()
