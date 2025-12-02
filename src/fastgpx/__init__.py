# Re-export the C extension module to make it available as fastgpx instead of fastgpx.fastgpx.
from .fastgpx import Bounds, Gpx, LatLong, Segment, TimeBounds, Track, parse  # type: ignore
from .fastgpx import geo, polyline  # type: ignore

# Ensure submodules are importable as fastgpx.geo and fastgpx.polyline.
import sys
sys.modules['fastgpx.geo'] = geo
sys.modules['fastgpx.polyline'] = polyline

# Needed for sphinx autosummary to pick up the re-exported C extension module.
__all__: list = ['Bounds', 'Gpx', 'LatLong', 'Segment',
                 'TimeBounds', 'Track', 'parse']
