import importlib.util
from pathlib import Path

import pytest

# The benchmark scripts are not part of the package and have no tests of their own, so they
# rotted silently once (#55). Importing them catches broken imports and module-level errors.
# They depend on the `benchmarks` dependency group, so skip rather than fail when it is absent.

BENCHMARKS_DIR = Path(__file__).resolve().parent.parent / 'benchmarks'


def import_script(name: str):
    spec = importlib.util.spec_from_file_location(name, BENCHMARKS_DIR / f'{name}.py')
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class TestBenchmarkScripts:

    def test_benchmark_polyline_imports(self):
        pytest.importorskip('colorama')
        pytest.importorskip('polyline')
        module = import_script('benchmark_polyline')
        assert Path(module.GPX_PATH).is_dir()

    def test_benchmark_gpx_imports(self):
        pytest.importorskip('colorama')
        pytest.importorskip('lxml')
        pytest.importorskip('gpxpy')
        import_script('benchmark_gpx')
