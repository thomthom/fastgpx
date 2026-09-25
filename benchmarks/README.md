# Benchmarks

Notes on fastgpx's speed and the scripts that measure it. For how to build and run the Catch2
benchmarks, see [Development.md](../Development.md).

## Notes

| Doc | Question it answers | Date | Status |
|---|---|---|---|
| [performance.md](performance.md) | How fast is Sleipnir's upload path now, and what did each speed change gain? | 2026-09-25 | current; older figures below its line |
| [build_settings.md](build_settings.md) | How are the wheels built, and why? | 2026-09-24 | current; older experiments below its line |
| [review_decisions.md](review_decisions.md) | What did the review of `dev/latlong-time` find and decide, with the full tables? | 2026-09-25 | the record of that review |
| [ingest_profile.md](ingest_profile.md) | Where does the fastgpx side of Sleipnir's upload spend its time? | 2026-09-24 | historical |
| [load_profile.md](load_profile.md) | Where does `LoadGpx` spend its time on Windows and Linux? | 2026-09-24 | historical |
| [nanobind3.md](nanobind3.md) | Does nanobind 3 in split mode pay off? | 2026-09-09 | historical numbers; split mode is still used |
| [datetime_parse.md](datetime_parse.md) | Which timestamp parser became `parse_gpx_time`? | 2026-09-09 | historical |
| [nanobind_vs_pybind11.md](nanobind_vs_pybind11.md) | How does fastgpx with nanobind compare with pybind11? | 2025-11-26 | historical |
| [gpx_parse.md](gpx_parse.md) | What did adding time bounds cost the parser? | 2024-10-28 | historical |

The date is when the note was last measured or updated, from `git log`. Each historical note says
at its top what has changed since.

## Scripts

The files for the corpus scripts are not in the repository. `gpx/sleipnir` and `gpx/TET` are
listed by MD5 in [corpus_manifest.json](corpus_manifest.json). Run each side of a comparison more
than once, and alternate them.

| Script | What it measures | How to run it |
|---|---|---|
| [benchmark_ingest.py](benchmark_ingest.py) | Sleipnir's upload path, step by step, in ns per point, for the `current`, `no_copy` and `lonlat` variants | `uv run benchmarks/benchmark_ingest.py run gpx/sleipnir -o before.json`, then `compare before.json after.json` |
| [benchmark_corpus.py](benchmark_corpus.py) | `load()`, `time_bounds()`, `length_2d()` and `length_3d()` per file over whole folders, and whether two builds give the same results | `uv run benchmarks/benchmark_corpus.py run DIR [DIR ...] -o before.json`, then `compare before.json after.json` |
| [corpus_manifest.py](corpus_manifest.py) | Checks that the local `gpx/sleipnir` and `gpx/TET` match the manifest | `uv run benchmarks/corpus_manifest.py verify` (`--full` also recounts points) |
| [benchmark_gpx.py](benchmark_gpx.py) | Total track length of `gpx/2024 Great Roadtrip` with fastgpx against gpxpy, `xml.etree` and lxml | `uv run --group benchmarks benchmarks/benchmark_gpx.py` |
| [benchmark_polyline.py](benchmark_polyline.py) | `fastgpx.polyline.encode` against the `polyline` package over `gpx/2024 Great Roadtrip` | `uv run --group benchmarks benchmarks/benchmark_polyline.py` |

Two more measure C++ alone, outside this folder:

- The Catch2 `[!benchmark]` cases in `fastgpx_test`, for example `"[!benchmark][datetime]"` and
  `"[!benchmark][polyline]"`.
- `profile_load` (`src/cpp/profile_load.cpp`), which loads a folder repeatedly under a profiler:
  `profile_load gpx/TET 20`. See [load_profile.md](load_profile.md).
