# Review decisions: `dev/latlong-time`

A review of the `dev/latlong-time` branch (LatLong and time parsing changes, link-time
optimization for the Linux wheels, and the benchmark write-ups in this folder) turned up a list
of items. They are handled one at a time on `dev/latlong-time-review`. This file records, per
item, what was found, what was done and why, and what was left alone on purpose, so the reasoning
survives into later sessions.

## Decisions made up front

- **The 183-file figure.** `performance.md` and `build_settings.md` quote a 183-file collection
  that nobody can trace. It is recorded as most likely all 145 files of Sleipnir's upload folder,
  duplicates included, plus the 34 TET country routes in `gpx/TET` and the 4 in Sleipnir's
  `gpx/tet` (145 + 34 + 4 = 183). Unconfirmed. Those folders hold 144 unique files, which matches
  neither the 140 nor the 154 quoted elsewhere; 140 does match `gpx/sleipnir` + `gpx/TET`.
- **Multi-run numbers go into `performance.md`**, not into rewritten commit messages. Old commits
  keep their single-run numbers.
- **LatLong equality** will compare at microsecond resolution and gain a value hash (option b),
  provided that costs nothing measurable on the production path. Done in a later item.
- **Link-time optimization on Linux.** Sleipnir's upload path never reads a point's `.time`, so
  the `parse_gpx_time` slowdown under LTO does not reach production. To be confirmed by
  measurement in a later item. Linux ARM wheels remain unmeasured.
- **Won't fix:** the difference in garbage-collector handling between the timing loops of
  `benchmark_corpus.py`, and the widening of time bounds at a 1 s clock edge (the values are
  correct).

## Item: benchmarks cannot be traced to their files

**Found.** The benchmark write-ups quote collections of 140, 154 and 183 files, but none says
which folders they were. The files are not in the repository, and the script behind
`ingest_profile.md` (`ingest_bench.py`) was kept outside it and is lost. Results could not be
reproduced or compared with new ones.

**Done.**

- `gpx/sleipnir/` holds one copy of each unique file (by MD5) from Sleipnir's local upload folder:
  106 of 145. It is git-ignored like `gpx/TET/`; the sdist already excludes all of `gpx`.
- `corpus_manifest.json` lists every file in `gpx/sleipnir` and `gpx/TET` with its MD5, size,
  track-point count and timed-point count, and where it came from. `corpus_manifest.py` rewrites
  it (`generate`) or checks the local folders against it (`verify`, `--full` to recount points).
- `benchmark_corpus.py` output now records the fastgpx version, how it was installed and the MD5
  of the loaded extension, the repository commit, Python, platform and machine, and each file's
  MD5. `compare` lists files found in only one run and says what set the totals are over. It
  still reads the older output.
- `benchmark_ingest.py` replaces the lost script. It repeats the fastgpx calls of Sleipnir's upload
  view and `create_gpx_file` without Django and times each step per point, best and median over
  N rounds, with the same run information and a `compare` mode. It times two variants on separate
  parses, alternating which goes first each round: the current path, with a new `LatLong` per
  point, and the path after thomthom/sleipnir#596, which builds the coordinates from the parsed
  points. Later items should judge the branch against both, since production is moving to the
  second. Freeing each segment's point lists is timed as its own step; otherwise it lands in the
  next segment's steps, or for the last segment is never timed. It is not small: about 47 ns per
  point with the copy and 29 without, on a Windows sample.
- `profile_load` now exits non-zero with a message for a missing folder or a file, a pass count
  that is not a positive integer, a file that fails to load, and a folder with no track points.

**Why.** Later items will quote numbers. They need to name their files by something checkable,
and they need to measure the path production runs, not only `load()`.

**Not done.**

- The old figures in `performance.md`, `build_settings.md` and `ingest_profile.md` were not
  re-measured or relabelled; that belongs to the item that re-runs them.
- `segment_to_polyline` is not in `benchmark_ingest.py`: in Sleipnir only its tests call it, so
  polyline encoding is not on the upload path.
- The manifest does not cover Sleipnir's `gpx/tet` (4 files) or the other folders under `gpx/`.
- `profile_load` was built and checked with MSVC only; it is not a parser change, so the
  sanitized Clang build was not run for it.
