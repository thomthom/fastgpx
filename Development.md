# Development

## Python

### Import Order

The typical organization of Python imports follows a structured convention to improve readability and maintainability of the code. This organization often adheres to the PEP 8 style guide, which is widely adopted in the Python community. Here is the recommended order and format:

Python imports are generally organized into three main sections, each separated by a blank line:

1. Standard Library Imports: These are modules that are part of Python's standard library, such as `os`, `sys`, `datetime`, etc.

2. Third-Party Imports: These are external libraries that are not part of the standard library, such as `numpy`, `requests`, etc.

3. Local Application or Project-Specific Imports: These are your own modules that are part of the project.

```py
# Standard library imports
import os
import sys
from datetime import datetime

# Third-party imports
import requests
import numpy as np

# Local application imports
from my_project.module import my_function
from . import another_module
```


### Python C Extension

The project is managed with `uv`. `uv sync` builds the C++ extension and installs it (as an
editable install) together with the dependency groups you ask for:

```sh
uv sync --all-groups
```

The extension is rebuilt automatically by `uv sync`/`uv run` when files under `src/cpp/` or
`CMakeLists.txt` change (see `[tool.uv] cache-keys` in `pyproject.toml`). To force a rebuild:

```sh
uv sync --reinstall-package fastgpx
```

By default uv builds the extension in an isolated environment, resolving `build-system.requires`
from PyPI. The `dev` group also contains the build tools (`scikit-build-core`, `nanobind`,
`cmake`, `ninja`), so the build can instead run inside the project `.venv`. This is useful in
sandboxes with limited network access and makes the build use the locked tool versions:

```sh
uv sync --all-groups --no-install-project                    # only needed on a brand new .venv
uv sync --all-groups --no-build-isolation-package fastgpx
```

Plain `pip install --editable .` still works too.

#### Linux

A C++23 compiler is required. On Ubuntu 24.04 the default GCC 13 is too old, so select GCC 14 or
Clang before the first build (CMake caches the compiler in the build directory):

```sh
export CC=gcc-14 CXX=g++-14        # or: export CC=clang CXX=clang++
export FASTGPX_WARNINGS_AS_ERRORS=ON   # same as CI
uv sync --all-groups
```

```sh
uv run python -m nanobind.stubgen fastgpx -o src -r -M typed.py
```

### Sphinx Documentation

```sh
uv sync --all-groups
uv sync --group docs
.venv\Scripts\activate
cd docs
make.bat html
```

Linux/macOS:

```sh
cd docs
uv run make html
# Force documentation rebuild / verbose output:
SPHINXOPTS="--fresh-env --verbose" uv run make html
```

The output is written to `docs/build/html`. The API docs are generated with autodoc, so the
extension must be built first.

```sh
# Force documentation rebuild:
set SPHINXOPTS=--fresh-env
```

```sh
set SPHINXOPTS=--verbose
```

```sh
set SPHINXOPTS=--fresh-env --verbose
```

### Python Benchmarking

```sh
uv run benchmarks/benchmark_gpx.py
```

```sh
uv run benchmarks/benchmark_polyline.py
```

### Python Profiling

https://learn.microsoft.com/en-us/visualstudio/python/profiling-python-code-in-visual-studio?view=vs-2022

```sh
snakeviz profiling/fastgpx_polyline_encode.prof
```

```sh
snakeviz profiling/polyline_encode.prof
```

### pyproject.toml

> Installing Dependencies with pyproject.toml
>
> You no longer need to use `pip install -r requirements.txt`. Instead, you can simply install dependencies directly using:
>
> ```sh
> pip install .
> ```

> For Development Dependencies:
>
> To install development dependencies (like `pytest` and `pybind11-stubgen`), you can use the `--extra` option (assuming you defined them under dev):
>
> ```sh
> pip install .[dev]
> ```
>
> This will install both the main dependencies and the development dependencies defined in the dev section of `pyproject.toml`.

### Locally test build wheel

```sh
pip install --upgrade build twine wheel
```

```sh
python -m build
twine check dist/*
```

## C++

### Include order

The organization of C++ includes follows certain conventions that are similar in spirit to Python import conventions. Well-structured includes can improve readability, reduce compile times, and minimize dependencies. Here are the typical guidelines and best practices for organizing C++ includes:

C++ includes are generally organized in a specific order, often grouped and separated by blank lines. The typical order is as follows:

1. Header File for the Current Implementation File (if applicable)
2. Standard Library Headers (e.g., `<iostream>`, `<vector>`)
3. Third-Party Library Headers (e.g., `boost`, or other external dependencies)
4. Project-Specific Headers (e.g., your own modules or classes)

This organization helps to ensure that your file includes what it needs directly, and minimizes the chance of accidentally relying on transitive includes from other files.

```cpp
// Current implementation file's corresponding header
#include "my_class.h"

// Standard library headers
#include <iostream>
#include <vector>
#include <string>

// Third-party library headers
#include <boost/algorithm/string.hpp>

// Project-specific headers
#include "utils.h"
#include "data_processing.h"
```

### Reformat all C++ sources

```sh
cd src\cpp
for /R %f in (*.cpp *.hpp) do "C:\Program Files\LLVM\bin\clang-format.exe" -i "%f"
```

Linux/macOS:

```sh
find src/cpp -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
```

### Coverage (C++ OpenCppCoverage)

```sh
coverage.bat ~[real_world]
```

### Catch2 Tests

If the output prints UTF-8 characters the terminal needs to be set to UTF-8 mode on Windows:

```sh
chcp 65001
```
#### Running Tests

```sh
build\src\cpp\RelWithDebInfo\fastgpx_test.exe
```

#### Running Benchmarks

```sh
build\src\cpp\RelWithDebInfo\fastgpx_test.exe [!benchmark]
```

#### Building and running from the command line (Linux/macOS)

This mirrors `.github/workflows/cpp-tests.yml`. `uv run cmake` uses the CMake from the `dev`
group, which satisfies the `cmake_minimum_required` version regardless of the system CMake. The
first configure clones pugixml, Catch2 and nlohmann/json with `FetchContent`.

```sh
uv run cmake -S . -B build-cpp -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
    -DFASTGPX_WARNINGS_AS_ERRORS=ON -DPython_EXECUTABLE="$PWD/.venv/bin/python"
uv run cmake --build build-cpp --parallel --target fastgpx_test
uv run ctest --test-dir build-cpp --output-on-failure

build-cpp/src/cpp/fastgpx_test                  # run the Catch2 binary directly
build-cpp/src/cpp/fastgpx_test "[!benchmark]"   # benchmarks
```

The tests compare against `src/cpp/expected_gpx_data.json`, which is generated from the Python
API with `uv run catch2.py`.

## VSCode / CMake

Building directly with CMake requires `nanobind` (and a recent enough `cmake`) to be importable
from the Python interpreter CMake is pointed at. Both are part of the `dev` dependency group, so
`uv sync` installs them into `.venv`. Building the `fastgpx` target directly with CMake also
copies the extension module into the site-packages directory of that interpreter.
