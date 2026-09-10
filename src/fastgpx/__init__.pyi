from collections.abc import (
    Iterable,
    Iterator,
    MutableSequence,
    Sequence
)
import datetime
import os
from typing import overload

from fastgpx import geo as geo, polyline as polyline


class Error(ValueError):
    pass

class ParseError(Error):
    pass

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

class LatLongList(MutableSequence[LatLong]):
    """
    The points of a :class:`Segment`, a mutable sequence of :class:`LatLong` backed by the segment's own storage. Editing ``segment.points`` edits the segment. ``LatLongList(iterable)`` builds one from any iterable of points, and a plain sequence of points is accepted wherever a ``LatLongList`` is expected.

    Indexing and iteration return *copies* of the points, and a slice is a new, independent ``LatLongList``. Assigning to an attribute of an element therefore does not change the segment. To change a point, assign it back::

        point = segment.points[0]
        point.latitude = 60.0
        segment.points[0] = point
    """

    @overload
    def __init__(self) -> None:
        """Default constructor"""

    @overload
    def __init__(self, arg: LatLongList, /) -> None:
        """Copy constructor"""

    @overload
    def __init__(self, arg: Iterable[LatLong], /) -> None:
        """Construct from an iterable object"""

    def __len__(self) -> int: ...

    def __bool__(self) -> bool:
        """Check whether the vector is nonempty"""

    def __repr__(self) -> str: ...

    def __iter__(self) -> Iterator[LatLong]: ...

    @overload
    def __getitem__(self, arg: int, /) -> LatLong: ...

    @overload
    def __getitem__(self, arg: slice, /) -> LatLongList: ...

    def clear(self) -> None:
        """Remove all items from list."""

    def append(self, arg: LatLong, /) -> None:
        """Append ``arg`` to the end of the list."""

    def insert(self, arg0: int, arg1: LatLong, /) -> None:
        """Insert object ``arg1`` before index ``arg0``."""

    def pop(self, index: int = -1) -> LatLong:
        """Remove and return item at ``index`` (default last)."""

    def extend(self, arg: LatLongList, /) -> None:
        """Extend ``self`` by appending elements from ``arg``."""

    @overload
    def __setitem__(self, arg0: int, arg1: LatLong, /) -> None: ...

    @overload
    def __setitem__(self, arg0: slice, arg1: LatLongList, /) -> None: ...

    @overload
    def __delitem__(self, arg: int, /) -> None: ...

    @overload
    def __delitem__(self, arg: slice, /) -> None: ...

    def __eq__(self, arg: object, /) -> bool: ...

    def __ne__(self, arg: object, /) -> bool: ...

    @overload
    def __contains__(self, arg: LatLong, /) -> bool: ...

    @overload
    def __contains__(self, arg: object, /) -> bool: ...

    @overload
    def index(self, value: LatLong, start: int = 0, stop: int | None = None) -> int: ...

    @overload
    def index(self, value: object, start: int = 0, stop: int | None = None) -> int: ...

    def reverse(self) -> None: ...

    def __reversed__(self) -> Iterator[LatLong]: ...

    @overload
    def count(self, value: LatLong) -> int: ...

    @overload
    def count(self, value: object) -> int: ...

    @overload
    def remove(self, value: LatLong) -> None: ...

    @overload
    def remove(self, value: object) -> None: ...

    def __iadd__(self, values: LatLongList | Iterable[LatLong]) -> LatLongList: ...

    __hash__: None = None

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
    def min(self, arg: LatLong | None, /) -> None: ...

    @property
    def max(self) -> LatLong | None: ...

    @max.setter
    def max(self, arg: LatLong | None, /) -> None: ...

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
    def points(self) -> LatLongList:
        """The track points as a :class:`LatLongList`."""

    @points.setter
    def points(self, arg: LatLongList | Sequence[LatLong], /) -> None: ...

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

class SegmentList(Sequence[Segment]):
    """
    The segments of a :class:`Track`, a read-only sequence of the track's own :class:`Segment` objects. Changes made through an element are visible in the track, and each element keeps its document alive.
    """

    def __len__(self) -> int: ...

    def __bool__(self) -> bool: ...

    @overload
    def __getitem__(self, index: int) -> Segment: ...

    @overload
    def __getitem__(self, index: slice) -> list[Segment]: ...

    def __iter__(self) -> Iterator[Segment]: ...

    def __reversed__(self) -> Iterator[Segment]: ...

    @overload
    def __contains__(self, value: Segment) -> bool: ...

    @overload
    def __contains__(self, value: object) -> bool: ...

    @overload
    def count(self, value: Segment) -> int: ...

    @overload
    def count(self, value: object) -> int: ...

    @overload
    def index(self, value: Segment, start: int = 0, stop: int | None = None) -> int: ...

    @overload
    def index(self, value: object, start: int = 0, stop: int | None = None) -> int: ...

    def __repr__(self) -> str: ...

class Track:
    """Represent ``<trk>`` data in GPX files."""

    def __init__(self) -> None: ...

    @property
    def name(self) -> str | None: ...

    @name.setter
    def name(self, arg: str | None, /) -> None: ...

    @property
    def comment(self) -> str | None: ...

    @comment.setter
    def comment(self, arg: str | None, /) -> None: ...

    @property
    def description(self) -> str | None: ...

    @description.setter
    def description(self, arg: str | None, /) -> None: ...

    @property
    def number(self) -> int | None: ...

    @number.setter
    def number(self, arg: int | None, /) -> None: ...

    @property
    def type(self) -> str | None: ...

    @type.setter
    def type(self, arg: str | None, /) -> None: ...

    @property
    def segments(self) -> SegmentList:
        """The segments as a :class:`SegmentList`."""

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

class TrackList(Sequence[Track]):
    """
    The tracks of a :class:`Gpx`, a read-only sequence of the document's own :class:`Track` objects. Changes made through an element are visible in the document, and each element keeps the document alive.
    """

    def __len__(self) -> int: ...

    def __bool__(self) -> bool: ...

    @overload
    def __getitem__(self, index: int) -> Track: ...

    @overload
    def __getitem__(self, index: slice) -> list[Track]: ...

    def __iter__(self) -> Iterator[Track]: ...

    def __reversed__(self) -> Iterator[Track]: ...

    @overload
    def __contains__(self, value: Track) -> bool: ...

    @overload
    def __contains__(self, value: object) -> bool: ...

    @overload
    def count(self, value: Track) -> int: ...

    @overload
    def count(self, value: object) -> int: ...

    @overload
    def index(self, value: Track, start: int = 0, stop: int | None = None) -> int: ...

    @overload
    def index(self, value: object, start: int = 0, stop: int | None = None) -> int: ...

    def __repr__(self) -> str: ...

class Gpx:
    """Represent ``<gpx>`` data in GPX files."""

    def __init__(self) -> None: ...

    @property
    def tracks(self) -> TrackList:
        """The tracks as a :class:`TrackList`."""

    @property
    def name(self) -> str | None: ...

    @name.setter
    def name(self, arg: str | None, /) -> None: ...

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

def load(path: str | bytes | os.PathLike[str] | os.PathLike[bytes]) -> Gpx:
    """
    Load and parse a GPX file.

    Releases the GIL while parsing, so files can be loaded from several threads at once.
    """

def parse(data: str) -> Gpx:
    """
    Parse GPX data from a string.

    Releases the GIL while parsing, so data can be parsed from several threads at once.
    """
