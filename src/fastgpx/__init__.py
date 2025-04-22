# Re-export the C extension module to make it available as fastgpx instead of fastgpx.fastgpx.
from .fastgpx import *  # type: ignore

# Needed for sphinx autosummary to pick up the re-exported C extension module.
__all__ = [name for name in dir(fastgpx) if not name.startswith('_')]  # type: ignore
