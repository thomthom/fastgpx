from __future__ import annotations
import fastgpx
import fastgpx.fastgpx
import typing
__all__ = ['Precision', 'decode', 'encode']
class Precision:
    """
    Members:
    
      Five
    
      Six
    """
    Five: typing.ClassVar[Precision]  # value = <Precision.Five: 5>
    Six: typing.ClassVar[Precision]  # value = <Precision.Six: 6>
    __members__: typing.ClassVar[dict[str, Precision]]  # value = {'Five': <Precision.Five: 5>, 'Six': <Precision.Six: 6>}
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
@typing.overload
def decode(encoded: str, precision: Precision = fastgpx.polyline.Precision.Five) -> list[fastgpx.fastgpx.LatLong]:
    ...
@typing.overload
def decode(locations: str, precision: int = 5) -> list[fastgpx.fastgpx.LatLong]:
    ...
@typing.overload
def encode(locations: list[fastgpx.fastgpx.LatLong], precision: Precision = fastgpx.polyline.Precision.Five) -> str:
    ...
@typing.overload
def encode(locations: list[fastgpx.fastgpx.LatLong], precision: int = 5) -> str:
    ...
