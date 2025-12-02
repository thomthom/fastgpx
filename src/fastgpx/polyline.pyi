from collections.abc import Sequence
import enum
from typing import overload

import fastgpx


class Precision(enum.Enum):
    Five = 5

    Six = 6

@overload
def encode(locations: Sequence[fastgpx.LatLong], precision: Precision = Precision.Five) -> str: ...

@overload
def encode(locations: Sequence[fastgpx.LatLong], precision: int = 5) -> str: ...

@overload
def decode(encoded: str, precision: Precision = Precision.Five) -> list[fastgpx.LatLong]: ...

@overload
def decode(locations: str, precision: int = 5) -> list[fastgpx.LatLong]: ...
