from collections.abc import Sequence
import datetime
from typing import overload

from . import geo as geo, polyline as polyline


class TimeBounds:
    @overload
    def __init__(self) -> None: ...

    @overload
    def __init__(self, start_time: datetime.datetime | datetime.date | datetime.time | None, end_time: datetime.datetime | datetime.date | datetime.time | None) -> None: ...

    @property
    def start_time(self) -> datetime.datetime | None: ...

    @start_time.setter
    def start_time(self, start_time: datetime.datetime | datetime.date | datetime.time | None) -> None: ...

    @property
    def end_time(self) -> datetime.datetime | None: ...

    @end_time.setter
    def end_time(self, end_time: datetime.datetime | datetime.date | datetime.time | None) -> None: ...

    def is_empty(self) -> bool: ...

    def is_range(self) -> bool: ...

    @overload
    def add(self, datetime: datetime.datetime | datetime.date | datetime.time) -> None: ...

    @overload
    def add(self, timebounds: TimeBounds) -> None: ...

    def __eq__(self, arg: object, /) -> bool: ...

    def __repr__(self) -> str: ...

    def __str__(self) -> str: ...

class LatLong:
    """Represent ``<trkpt>`` data in GPX files."""

    @overload
    def __init__(self) -> None: ...

    @overload
    def __init__(self, latitude: float, longitude: float, elevation: float = 0.0) -> None: ...

    @property
    def latitude(self) -> float:
        """The latitude of the point. Decimal degrees, WGS84 datum."""

    @latitude.setter
    def latitude(self, arg: float, /) -> None: ...

    @property
    def longitude(self) -> float:
        """The longitude of the point. Decimal degrees, WGS84 datum."""

    @longitude.setter
    def longitude(self, arg: float, /) -> None: ...

    @property
    def elevation(self) -> float:
        """The elevation of the point in meters."""

    @elevation.setter
    def elevation(self, arg: float, /) -> None: ...

    def __eq__(self, arg: object, /) -> bool: ...

    def __repr__(self) -> str: ...

    def __str__(self) -> str: ...

class Bounds:
    @overload
    def __init__(self) -> None: ...

    @overload
    def __init__(self, min: LatLong, max: LatLong) -> None: ...

    @overload
    def __init__(self, min: tuple[float, float], max: tuple[float, float]) -> None: ...

    @overload
    def __init__(self, min: tuple[float, float, float], max: tuple[float, float, float]) -> None: ...

    @property
    def min(self) -> LatLong | None: ...

    @min.setter
    def min(self, arg: LatLong, /) -> None: ...

    @property
    def max(self) -> LatLong | None: ...

    @max.setter
    def max(self, arg: LatLong, /) -> None: ...

    def is_empty(self) -> bool: ...

    @overload
    def add(self, location: LatLong) -> None: ...

    @overload
    def add(self, bounds: Bounds) -> None: ...

    def max_bounds(self, bounds: Bounds) -> Bounds: ...

    @property
    def min_latitude(self) -> float | None:
        """
        .. warning::

           Compatibility with ``gpxpy.GPXBounds.min_latitude``.
           Prefer ``min.latitude`` instead. :func:`min`
        """

    @min_latitude.setter
    def min_latitude(self, arg: float, /) -> None: ...

    @property
    def min_longitude(self) -> float | None:
        """
        .. warning::

           Compatibility with ``gpxpy.GPXBounds.min_longitude``.
           Prefer ``min.longitude`` instead. :func:`min`
        """

    @min_longitude.setter
    def min_longitude(self, arg: float, /) -> None: ...

    @property
    def max_latitude(self) -> float | None:
        """
        .. warning::

           Compatibility with ``gpxpy.GPXBounds.max_latitude``.
           Prefer ``max.latitude`` instead. :func:`max`
        """

    @max_latitude.setter
    def max_latitude(self, arg: float, /) -> None: ...

    @property
    def max_longitude(self) -> float | None:
        """
        .. warning::

           Compatibility with ``gpxpy.GPXBounds.max_longitude``.
           Prefer ``max.longitude`` instead. :func:`max`
        """

    @max_longitude.setter
    def max_longitude(self, arg: float, /) -> None: ...

    def __eq__(self, arg: object, /) -> bool: ...

    def __repr__(self) -> str: ...

    def __str__(self) -> str: ...

class Segment:
    """Represent ``<trkseg>`` data in GPX files."""

    def __init__(self) -> None: ...

    @property
    def points(self) -> list[LatLong]: ...

    @points.setter
    def points(self, arg: Sequence[LatLong], /) -> None: ...

    def bounds(self) -> Bounds: ...

    def get_bounds(self) -> Bounds:
        """
        .. warning::

           Compatibility with ``gpxpy.GPXTrackSegment.get_bounds``.
           Prefer :func:`bounds` instead.
        """

    def time_bounds(self) -> TimeBounds: ...

    def get_time_bounds(self) -> TimeBounds:
        """
        .. warning::

           Compatibility with ``gpxpy.GPXTrackSegment.get_time_bounds``.
           Prefer :func:`time_bounds` instead.
        """

    def length_2d(self) -> float:
        """Distance in meters."""

    def length_3d(self) -> float:
        """Distance in meters."""

    def __repr__(self) -> str: ...

class Track:
    """Represent ``<trk>`` data in GPX files."""

    def __init__(self) -> None: ...

    @property
    def name(self) -> str | None: ...

    @name.setter
    def name(self, arg: str, /) -> None: ...

    @property
    def comment(self) -> str | None: ...

    @comment.setter
    def comment(self, arg: str, /) -> None: ...

    @property
    def description(self) -> str | None: ...

    @description.setter
    def description(self, arg: str, /) -> None: ...

    @property
    def number(self) -> int | None: ...

    @number.setter
    def number(self, arg: int, /) -> None: ...

    @property
    def type(self) -> str | None: ...

    @type.setter
    def type(self, arg: str, /) -> None: ...

    @property
    def segments(self) -> list[Segment]: ...

    @segments.setter
    def segments(self, arg: Sequence[Segment], /) -> None: ...

    def bounds(self) -> Bounds: ...

    def get_bounds(self) -> Bounds:
        """
        .. warning::

           Compatibility with ``gpxpy.GPXTrack.get_bounds``.
           Prefer :func:`bounds` instead.
        """

    def time_bounds(self) -> TimeBounds: ...

    def get_time_bounds(self) -> TimeBounds:
        """
        .. warning::

           Compatibility with ``gpxpy.GPXTrack.get_time_bounds``.
           Prefer :func:`time_bounds` instead.
        """

    def length_2d(self) -> float:
        """Distance in meters."""

    def length_3d(self) -> float:
        """Distance in meters."""

    def __repr__(self) -> str: ...

class Gpx:
    """Represent ``<gpx>`` data in GPX files."""

    def __init__(self) -> None: ...

    @property
    def tracks(self) -> list[Track]: ...

    @tracks.setter
    def tracks(self, arg: Sequence[Track], /) -> None: ...

    @property
    def name(self) -> str | None: ...

    @name.setter
    def name(self, arg: str, /) -> None: ...

    def bounds(self) -> Bounds: ...

    def get_bounds(self) -> Bounds:
        """
        .. warning::

           Compatibility with ``gpxpy.GPX.get_bounds``.
           Prefer :func:`bounds` instead.
        """

    def time_bounds(self) -> TimeBounds: ...

    def get_time_bounds(self) -> TimeBounds:
        """
        .. warning::

           Compatibility with ``gpxpy.GPX.get_time_bounds``.
           Prefer :func:`time_bounds` instead.
        """

    def length_2d(self) -> float:
        """Distance in meters."""

    def length_3d(self) -> float:
        """Distance in meters."""

    def __repr__(self) -> str: ...

def parse(path: str) -> Gpx: ...
