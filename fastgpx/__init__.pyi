from __future__ import annotations
import typing
from . import polyline
__all__ = ['Gpx', 'LatLong', 'Segment', 'Track', 'parse', 'polyline']
class Gpx:
    tracks: list[Track]
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
    def length_2d(self) -> float:
        ...
    def length_3d(self) -> float:
        ...
class LatLong:
    elevation: float
    latitude: float
    longitude: float
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
class Segment:
    points: list[LatLong]
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
    def length_2d(self) -> float:
        ...
    def length_3d(self) -> float:
        ...
class Track:
    comment: str | None
    description: str | None
    name: str | None
    number: int | None
    segments: list[Segment]
    type: str | None
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
    def length_2d(self) -> float:
        ...
    def length_3d(self) -> float:
        ...
def parse(path: str) -> Gpx:
    ...
