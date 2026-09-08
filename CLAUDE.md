# fastgpx

Python library for parsing GPX files fast. The core is C++23 (`src/cpp/fastgpx/`, XML via
pugixml) exposed to Python through a nanobind module (`src/cpp/python_fastgpx.cpp`) and built
with scikit-build-core. `src/fastgpx/*.pyi` are type stubs *generated* by the build
(`nanobind_add_stub` in `src/cpp/CMakeLists.txt`); never hand-edit them, rebuild and commit.

Supported platforms are Windows (MSVC, clang-cl) and Linux (GCC 14+, Clang 18+). Keep changes
portable to both. See `Development.md` for the longer developer notes.

## Environment and build

Everything goes through `uv` (lockfile: `uv.lock`, Python 3.12 pinned in `.python-version`).

```sh
# Build the extension and install everything (tests, benchmarks, docs tooling):
uv sync --all-groups

# Same, but build with the tools from the `dev` group instead of an isolated build
# environment (no PyPI access needed once the venv exists). Preferred in sandboxes.
uv sync --all-groups --no-install-project                    # only on a brand new venv
uv sync --all-groups --no-build-isolation-package fastgpx
```

- A C++23 compiler is required. Ubuntu 24.04's default GCC 13 is too old: set
  `CC=gcc-14 CXX=g++-14` (or `CC=clang CXX=clang++`) before the first build. CI builds with
  `FASTGPX_WARNINGS_AS_ERRORS=ON`; set it locally so warnings surface as errors.
- CMake >= 3.30.2 is required. The `dev` group installs `cmake` and `ninja` into the venv, so use
  `uv run cmake` / `uv run ctest` rather than a possibly older system CMake.
- `uv run` and `uv sync` rebuild the extension automatically when files under `src/cpp/` or
  `CMakeLists.txt` change (`[tool.uv] cache-keys` in `pyproject.toml`). To force it:
  `uv sync --reinstall-package fastgpx`.
- nanobind is pinned to `<3`: the custom datetime caster in
  `src/cpp/python_utc_chrono_nanobind.hpp` relies on a header nanobind 3 removed.

## Tests

```sh
# Python tests (run from the repository root; test data paths are relative to it).
uv run pytest
uv run pytest -m "slow or gpxcompare"   # opt in to the markers deselected by pytest.ini

# C++ Catch2 tests.
uv run cmake -S . -B build-cpp -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
    -DFASTGPX_WARNINGS_AS_ERRORS=ON -DPython_EXECUTABLE="$PWD/.venv/bin/python"
uv run cmake --build build-cpp --parallel --target fastgpx_test
uv run ctest --test-dir build-cpp --output-on-failure
build-cpp/src/cpp/fastgpx_test "[!benchmark]"   # Catch2 benchmarks
```

- `pytest.ini` deselects `slow`, `wip` and `gpxcompare` tests by default.
- `test_parse_is_locale_independent` skips unless a locale with `,` as decimal separator (for
  example `de_DE.UTF-8`) is installed.
- The C++ tests compare against `src/cpp/expected_gpx_data.json`, generated from the Python API
  by `uv run catch2.py`. Regenerate it when parsing results legitimately change.
- Test GPX data lives in `gpx/` (real recordings under `gpx/2024 *`, small cases under
  `gpx/test/`, third-party samples under `gpx/third-party/`).
- The CMake configure step fetches pugixml, Catch2 and nlohmann/json from GitHub via
  `FetchContent`; set `FETCHCONTENT_BASE_DIR` to reuse the clones between build directories.

## Documentation

```sh
cd docs && uv run make html      # output in docs/build/html; needs the built extension
```

## Code style

- Python: PEP 8, 100 columns, imports grouped stdlib / third-party / local (see
  `Development.md`). Type hints everywhere; `mypy` uses the stubs in `stubs/`.
- C++: format with `clang-format` (`.clang-format`), includes ordered own header / standard
  library / third-party / project. Builds use `-Wall -Wextra -pedantic -Wshadow -Wconversion`.
- Commit messages: short imperative subject, as in `git log`.
