import datetime
import gc
import locale
from collections.abc import MutableSequence, Sequence
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import gpxpy
import pytest

import fastgpx


@pytest.fixture
def gpx_path():
    return "gpx/2024 TopCamp/Connected_20240518_094959_.gpx"


@pytest.fixture
def gpx_unicode_path():
    return "gpx/2024 Great Roadtrip/Connected_20240731_113605_Näsåker_Sollefteå.gpx"


@pytest.fixture
def gpx_japanese_unicode_path():
    return "gpx/test/テスト.gpx"


@pytest.fixture
def expected_gpx(gpx_path: str):
    with open(gpx_path, 'r', encoding='utf-8') as gpx_file:
        gpx = gpxpy.parse(gpx_file)
    return gpx


METERS_TOL = 1e-4


class TestGpx:

    # fastgpx.Gpx.length_2d

    def test_length2d(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        distance = gpx.length_2d()
        assert distance == pytest.approx(382952.7193, abs=METERS_TOL)

    # fastgpx.Gpx.name

    def test_gpx_name_missing(self):
        path = 'gpx/test/debug-segment.gpx'
        gpx = fastgpx.load(path)
        assert gpx.name is None

    def test_gpx_name_empty_string(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        assert gpx.name == ''

    def test_gpx_name(self):
        path = 'gpx/test/two-points.gpx'
        gpx = fastgpx.load(path)
        assert gpx.name == 'Two Point Segment'

    # fastgpx.Gpx.bounds

    def test_bounds(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        bounds = gpx.bounds()
        assert not bounds.is_empty()
        assert bounds.min is not None
        assert bounds.min.latitude == pytest.approx(61.410713)
        assert bounds.min.longitude == pytest.approx(10.427408)
        assert bounds.max is not None
        assert bounds.max.latitude == pytest.approx(63.441189)
        assert bounds.max.longitude == pytest.approx(13.142774)

    # fastgpx.Gpx.time_bounds

    def test_time_bounds(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        time_bounds = gpx.time_bounds()
        assert not time_bounds.is_empty()

        # 2024-05-18T07:50:00Z
        assert time_bounds.start_time is not None
        assert time_bounds.start_time == datetime.datetime(
            year=2024, month=5, day=18, hour=7, minute=50, second=0, tzinfo=datetime.timezone.utc)
        assert time_bounds.start_time.tzinfo is datetime.timezone.utc
        assert time_bounds.start_time.isoformat() == '2024-05-18T07:50:00+00:00'
        assert time_bounds.start_time.timestamp() == 1716018600

        # 2024-05-18T16:46:18Z
        assert time_bounds.end_time is not None
        assert time_bounds.end_time == datetime.datetime(
            year=2024, month=5, day=18, hour=16, minute=46, second=18, tzinfo=datetime.timezone.utc)
        assert time_bounds.end_time.tzinfo is datetime.timezone.utc
        assert time_bounds.end_time.isoformat() == '2024-05-18T16:46:18+00:00'
        assert time_bounds.end_time.timestamp() == 1716050778

    # fastgpx.Gpx.__repr__

    def test_repr(self):
        gpx = fastgpx.load("gpx/test/two-points.gpx")
        repr_str = repr(gpx)
        assert repr_str == "<fastgpx.Gpx(tracks: 1, name: 'Two Point Segment')>"

    def test_repr_no_filename(self):
        gpx = fastgpx.load("gpx/test/debug-segment.gpx")
        repr_str = repr(gpx)
        assert repr_str == "<fastgpx.Gpx(tracks: 1)>"

    # fastgpx.load

    def test_load_pathlib_path(self, gpx_path: str):
        path = Path(gpx_path)
        gpx = fastgpx.load(path)
        distance = gpx.length_2d()
        assert distance == pytest.approx(382952.7193, abs=METERS_TOL)

    def test_load_pathlib_path_unicode(self, gpx_unicode_path: str):
        path = Path(gpx_unicode_path)
        gpx = fastgpx.load(path)
        distance = gpx.length_2d()
        assert distance == pytest.approx(407621.5043, abs=METERS_TOL)

    def test_load_pathlib_path_japanese_unicode(self, gpx_japanese_unicode_path: str):
        path = Path(gpx_japanese_unicode_path)
        gpx = fastgpx.load(path)
        distance = gpx.length_2d()
        assert distance == pytest.approx(17809.2701, abs=METERS_TOL)

    # fastgpx.parse

    def test_parse(self, gpx_path: str):
        with open(gpx_path, 'r', encoding='utf-8') as gpx_file:
            gpx_data = gpx_file.read()
        gpx = fastgpx.parse(gpx_data)
        distance = gpx.length_2d()
        assert distance == pytest.approx(382952.7193, abs=METERS_TOL)

    def test_parse_is_locale_independent(self):
        # Locales that use ',' as the decimal separator. Names differ between platforms.
        candidates = ['de_DE.UTF-8', 'de_DE.utf8', 'de_DE', 'German_Germany.1252', 'nb_NO.UTF-8']
        original = locale.setlocale(locale.LC_NUMERIC)
        for name in candidates:
            try:
                locale.setlocale(locale.LC_NUMERIC, name)
                break
            except locale.Error:
                continue
        else:
            pytest.skip('No locale with "," decimal separator available')
        try:
            # Guard against a locale that exists but does not actually change the separator,
            # in which case the test would pass without exercising anything.
            if locale.localeconv()['decimal_point'] != ',':
                pytest.skip('Selected locale does not use "," as decimal separator')
            xml = ('<gpx><trk><trkseg>'
                   '<trkpt lat="61.5" lon="10.25"><ele>123.5</ele></trkpt>'
                   '<trkpt lat=" +61.75" lon="10.5"></trkpt>'
                   '</trkseg></trk></gpx>')
            gpx = fastgpx.parse(xml)
        finally:
            locale.setlocale(locale.LC_NUMERIC, original)
        points = gpx.tracks[0].segments[0].points
        assert points[0] == fastgpx.LatLong(61.5, 10.25, 123.5)
        assert points[1] == fastgpx.LatLong(61.75, 10.5, 0.0)

    def test_parse_invalid_elevation_is_zero(self):
        # An elevation that cannot be represented as a double (here: overflow) is treated like
        # a missing one and yields 0.0 rather than infinity. What it should be is #70.
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><ele>-1e999</ele></trkpt>'
               '</trkseg></trk></gpx>')
        gpx = fastgpx.parse(xml)
        assert gpx.tracks[0].segments[0].points[0] == fastgpx.LatLong(60.5, 10.5, 0.0)

    @pytest.mark.parametrize('trkpt, message', [
        ('<trkpt lon="10.5"/>', 'missing the lat attribute'),
        ('<trkpt lat="60.5"/>', 'missing the lon attribute'),
        ('<trkpt lat="abc" lon="10.5"/>', 'lat attribute is not a valid number'),
        ('<trkpt lat="60.5" lon=""/>', 'lon attribute is not a valid number'),
        ('<trkpt lat="1e999" lon="10.5"/>', 'lat attribute is not a valid number'),
        ('<trkpt lat="90.5" lon="10.5"/>', 'lat attribute is out of range'),
        ('<trkpt lat="60.5" lon="-180.5"/>', 'lon attribute is out of range'),
        ('<trkpt lat="nan" lon="10.5"/>', 'lat attribute is out of range'),
        ('<trkpt lat="60.5" lon="inf"/>', 'lon attribute is out of range'),
    ])
    def test_parse_invalid_coordinate_raises_parse_error(self, trkpt: str, message: str):
        # A <trkpt> with a missing, unparseable or out-of-range lat/lon used to become a point at
        # (0, 0), which silently added thousands of km to length_2d().
        xml = f'<gpx><trk><trkseg>{trkpt}</trkseg></trk></gpx>'
        with pytest.raises(fastgpx.ParseError, match=message):
            fastgpx.parse(xml)


class TestTrack:

    # fastgpx.Track.length_2d

    def test_simple_track_length2d(self):
        path = 'gpx/test/debug-segment.gpx'
        gpx = fastgpx.load(path)
        # Assigning intermediate values for easier debug inspection.
        tracks = gpx.tracks
        track = tracks[0]
        distance = track.length_2d()
        assert distance == pytest.approx(1.3839, abs=METERS_TOL)

    def test_length2d(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        track = gpx.tracks[0]
        distance = track.length_2d()
        assert distance == pytest.approx(382952.7193, abs=METERS_TOL)

    # fastgpx.Track.__repr__

    def test_repr(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        track = gpx.tracks[0]
        repr_str = repr(track)
        assert repr_str == "<fastgpx.Track(segments: 9)>"


class TestSegment:

    # fastgpx.Segment.length_2d

    def test_simple_segment_length2d(self):
        path = 'gpx/test/debug-segment.gpx'
        gpx = fastgpx.load(path)
        # Assigning intermediate values for easier debug inspection.
        tracks = gpx.tracks
        track = tracks[0]
        segments = track.segments
        segment = segments[0]
        distance = segment.length_2d()
        assert distance == pytest.approx(1.3839, abs=METERS_TOL)

    def test_length2d(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        track = gpx.tracks[0]
        segment = track.segments[0]
        distance = segment.length_2d()
        assert distance == pytest.approx(17809.2701, abs=METERS_TOL)

    # fastgpx.Segment.__repr__

    def test_repr(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        segment = gpx.tracks[0].segments[0]
        repr_str = repr(segment)
        assert repr_str == "<fastgpx.Segment(points: 1326)>"


class TestContainers:
    # points, segments and tracks expose the C++ vectors directly instead of converting them to a
    # list on every access. Points are returned as copies so that the mutating methods can
    # never leave a dangling wrapper; segments and tracks are references and read-only.

    def test_container_types(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        assert isinstance(gpx.tracks, fastgpx.TrackList)
        assert isinstance(gpx.tracks[0].segments, fastgpx.SegmentList)
        assert isinstance(gpx.tracks[0].segments[0].points, fastgpx.LatLongList)

    def test_len_and_index(self, gpx_path: str):
        points = fastgpx.load(gpx_path).tracks[0].segments[0].points
        assert len(points) == 1326
        assert points[-1] == points[len(points) - 1]
        with pytest.raises(IndexError):
            points[len(points)]
        with pytest.raises(IndexError):
            fastgpx.parse('<gpx/>').tracks[0]

    def test_empty_document(self):
        tracks = fastgpx.parse('<gpx/>').tracks
        assert len(tracks) == 0
        assert not tracks
        assert list(tracks) == []

    def test_append_modifies_segment(self):
        segment = fastgpx.Segment()
        segment.points.append(fastgpx.LatLong(60.0, 10.0))
        segment.points.append(fastgpx.LatLong(60.1, 10.1))
        assert len(segment.points) == 2
        assert segment.length_2d() == pytest.approx(12431.7, abs=1.0)

    def test_nested_append_is_visible_in_document(self):
        gpx = fastgpx.parse('<gpx><trk><trkseg><trkpt lat="60" lon="10"/></trkseg></trk></gpx>')
        gpx.tracks[0].segments[0].points.append(fastgpx.LatLong(60.1, 10.1))
        assert len(gpx.tracks[0].segments[0].points) == 2

    def test_elements_are_copies(self):
        # Attribute assignment on an element changes a temporary, by design. Replacing the
        # element is the way to change a point.
        segment = fastgpx.Segment()
        segment.points.append(fastgpx.LatLong(60.0, 10.0))
        segment.points[0].latitude = 61.0
        assert segment.points[0].latitude == 60.0
        point = segment.points[0]
        point.latitude = 61.0
        segment.points[0] = point
        assert segment.points[0].latitude == 61.0

    def test_points_setter_accepts_sequence(self):
        segment = fastgpx.Segment()
        segment.points = [fastgpx.LatLong(60.0, 10.0), fastgpx.LatLong(60.1, 10.1)]
        assert len(segment.points) == 2
        segment.points = fastgpx.LatLongList([fastgpx.LatLong(60.0, 10.0)])
        assert len(segment.points) == 1

    def test_slice_and_list(self, gpx_path: str):
        points = fastgpx.load(gpx_path).tracks[0].segments[0].points
        first = points[:3]
        assert isinstance(first, fastgpx.LatLongList)
        assert list(first) == [points[0], points[1], points[2]]
        assert isinstance(list(points), list)

    def test_segments_and_tracks_are_read_only(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        with pytest.raises(AttributeError):
            gpx.tracks = []  # type: ignore[misc]
        with pytest.raises(AttributeError):
            gpx.tracks[0].segments = []  # type: ignore[misc]
        assert not hasattr(gpx.tracks, 'append')
        assert not hasattr(gpx.tracks[0].segments, 'append')

    def test_track_edits_are_visible_in_document(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        gpx.tracks[0].name = 'renamed'
        assert gpx.tracks[0].name == 'renamed'
        assert [track.name for track in gpx.tracks][0] == 'renamed'

    def test_element_keeps_document_alive(self, gpx_path: str):
        # The document is dropped as soon as the expression completes; the track and segment
        # must keep it alive rather than refer into freed memory.
        expected = fastgpx.load(gpx_path).length_2d()
        track = fastgpx.load(gpx_path).tracks[0]
        segment = fastgpx.load(gpx_path).tracks[0].segments[0]
        for _ in range(3):
            fastgpx.load(gpx_path)
        gc.collect()
        assert track.length_2d() == pytest.approx(expected, abs=METERS_TOL)
        assert len(segment.points) == 1326

    def test_iteration_over_document(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        count = sum(len(segment.points) for track in gpx.tracks for segment in track.segments)
        assert count == sum(len(list(segment.points))
                            for track in gpx.tracks for segment in track.segments)
        assert count == 19962

    def test_abc_registration(self, gpx_path: str):
        gpx = fastgpx.load(gpx_path)
        assert isinstance(gpx.tracks, Sequence)
        assert isinstance(gpx.tracks[0].segments, Sequence)
        assert isinstance(gpx.tracks[0].segments[0].points, MutableSequence)
        assert not isinstance(gpx.tracks, MutableSequence)

    def test_points_sequence_protocol(self):
        a, b, c = fastgpx.LatLong(60.0, 10.0), fastgpx.LatLong(60.1, 10.1), fastgpx.LatLong(60.2, 10.2)
        points = fastgpx.LatLongList([a, b, a])
        assert b in points
        assert c not in points
        assert 'not a point' not in points
        assert points.count(a) == 2
        assert points.index(a) == 0
        assert points.index(a, 1) == 2
        with pytest.raises(ValueError):
            points.index(c)
        with pytest.raises(ValueError):
            points.index(b, 2)
        points += [c]
        assert list(points) == [a, b, a, c]
        points += points
        assert len(points) == 8

    def test_points_keyword_arguments(self):
        # Generic MutableSequence code passes these by keyword.
        a, b = fastgpx.LatLong(60.0, 10.0), fastgpx.LatLong(60.1, 10.1)
        points = fastgpx.LatLongList([a, b, a])
        assert points.count(value=a) == 2
        assert points.index(value=b, start=0, stop=None) == 1
        points.remove(value=a)
        assert list(points) == [b, a]

    def test_points_unhashable(self):
        assert fastgpx.LatLongList([fastgpx.LatLong(60.0, 10.0)]) == \
            fastgpx.LatLongList([fastgpx.LatLong(60.0, 10.0)])
        with pytest.raises(TypeError):
            hash(fastgpx.LatLongList())

    def test_points_slice_is_independent(self):
        segment = fastgpx.Segment()
        segment.points.extend([fastgpx.LatLong(60.0, 10.0), fastgpx.LatLong(60.1, 10.1)])
        head = segment.points[:1]
        head.append(fastgpx.LatLong(60.2, 10.2))
        assert len(head) == 2
        assert len(segment.points) == 2

    def test_points_foreign_values(self):
        # A value that is not a point answers as it would for a list: absent, not a TypeError.
        points = fastgpx.LatLongList([fastgpx.LatLong(60.0, 10.0)])
        assert points.count('not a point') == 0
        with pytest.raises(ValueError):
            points.index('not a point')
        with pytest.raises(ValueError):
            points.remove('not a point')

    def test_points_reverse(self):
        a, b, c = fastgpx.LatLong(60.0, 10.0), fastgpx.LatLong(60.1, 10.1), fastgpx.LatLong(60.2, 10.2)
        points = fastgpx.LatLongList([a, b, c])
        assert list(reversed(points)) == [c, b, a]
        points.reverse()
        assert list(points) == [c, b, a]

    def test_points_mutators_edit_segment(self):
        a, b, c, d = (fastgpx.LatLong(60.0, 10.0), fastgpx.LatLong(60.1, 10.1),
                      fastgpx.LatLong(60.2, 10.2), fastgpx.LatLong(60.3, 10.3))
        segment = fastgpx.Segment()
        segment.points.extend([a, b, c])
        segment.points.insert(1, d)
        assert list(segment.points) == [a, d, b, c]
        assert segment.points.pop() == c
        assert segment.points.pop(0) == a
        assert list(segment.points) == [d, b]
        segment.points.remove(d)
        assert list(segment.points) == [b]
        del segment.points[0]
        assert len(segment.points) == 0
        segment.points.extend([a, b, c, d])
        segment.points[1:3] = [d, a]
        assert list(segment.points) == [a, d, a, d]
        del segment.points[::2]
        assert list(segment.points) == [d, d]
        segment.points.clear()
        assert len(segment.points) == 0
        assert segment.length_2d() == 0.0

    def test_points_mutation_during_iteration(self):
        # Resizing the vector while an iterator is alive must not read freed memory. Forward
        # iteration reads the live vector by index, so appended points are visited and a cleared
        # vector ends the loop. reversed() iterates a copy taken when it was called.
        a, b = fastgpx.LatLong(60.0, 10.0), fastgpx.LatLong(60.1, 10.1)
        points = fastgpx.LatLongList([a, b])
        seen = []
        for point in points:
            seen.append(point)
            if len(points) < 1000:
                points.append(b)
        assert len(seen) == 1000
        forward = iter(points)
        backward = reversed(points)
        assert next(forward) == a
        assert next(backward) == b
        points.clear()
        with pytest.raises(StopIteration):
            next(forward)
        assert next(backward) == b
        assert list(backward)[-1] == a

    def test_read_only_sequence_protocol(self, gpx_path: str):
        # Segment and Track have no equality, so membership is by identity, as it would be for
        # Python objects without __eq__.
        gpx = fastgpx.load(gpx_path)
        segments = gpx.tracks[0].segments
        second = segments[1]
        assert second in segments
        assert fastgpx.Segment() not in segments
        assert 'not a segment' not in segments
        assert segments.count('not a segment') == 0
        with pytest.raises(ValueError):
            segments.index('not a segment')
        assert segments.count(second) == 1
        assert segments.index(second) == 1
        assert segments.index(second, -8) == 1
        with pytest.raises(ValueError):
            segments.index(second, 2)
        with pytest.raises(ValueError):
            segments.index(fastgpx.Segment())
        assert [len(s.points) for s in reversed(segments)] == \
            [len(s.points) for s in segments][::-1]

    def test_read_only_slice_is_list_of_document_objects(self, gpx_path: str):
        segments = fastgpx.load(gpx_path).tracks[0].segments
        tail = segments[-2:]
        assert isinstance(tail, list)
        assert len(tail) == 2
        assert tail[1] is segments[-1]
        assert segments[::3] == [segments[i] for i in range(0, len(segments), 3)]
        assert segments[5:2] == []


class TestErrors:
    # All fastgpx errors describe input the library cannot use, so they are ValueError
    # subclasses: fastgpx.Error is the base and fastgpx.ParseError covers malformed data.
    # File access failures are OSErrors.

    def test_error_hierarchy(self):
        assert issubclass(fastgpx.Error, ValueError)
        assert issubclass(fastgpx.ParseError, fastgpx.Error)

    def test_parse_malformed_gpx_raises_parse_error(self):
        with pytest.raises(fastgpx.ParseError, match='Failed to parse GPX data'):
            fastgpx.parse('<gpx><trk>')

    def test_parse_malformed_gpx_is_value_error(self):
        with pytest.raises(ValueError):
            fastgpx.parse('<gpx><trk>')

    def test_parse_embedded_nul_raises_parse_error(self):
        # U+0000 is not a valid XML character. The underlying XML parser stops at it silently, so
        # without an explicit check this would return a truncated document reporting success.
        data = ('<gpx><trk><trkseg><trkpt lat="60" lon="10"/>\x00'
                '<trkpt lat="60.1" lon="10"/></trkseg></trk></gpx>')
        with pytest.raises(fastgpx.ParseError, match='NUL byte'):
            fastgpx.parse(data)

    def test_parse_nul_after_complete_document_raises_parse_error(self):
        # The prefix alone is a well-formed document, so the truncation would otherwise be silent.
        doc = '<gpx><trk><trkseg><trkpt lat="60" lon="10"/></trkseg></trk></gpx>'
        with pytest.raises(fastgpx.ParseError, match='NUL byte'):
            fastgpx.parse(doc + '\x00' + doc)

    def test_parse_without_gpx_root_raises_parse_error(self):
        # Well-formed XML of some other type (TCX, KML, a saved HTML error page) used to parse as
        # a Gpx with no tracks and no error.
        with pytest.raises(fastgpx.ParseError, match='missing <gpx> root element'):
            fastgpx.parse('<html><body/></html>')

    def test_parse_empty_gpx_root_is_empty_document(self):
        assert len(fastgpx.parse('<gpx/>').tracks) == 0

    def test_load_missing_file_raises_file_not_found(self):
        with pytest.raises(FileNotFoundError, match='not-a-real-path'):
            fastgpx.load('gpx/not-a-real-path/fake.gpx')

    def test_load_directory_raises_os_error(self):
        with pytest.raises(OSError):
            fastgpx.load('gpx/test')

    def test_load_nul_after_complete_document_raises_parse_error(self, tmp_path: Path):
        # Same check as parse(): the prefix alone is a well-formed document, so the truncation
        # would otherwise be silent. NUL padding is what a partially written file looks like.
        doc = b'<gpx><trk><trkseg><trkpt lat="60" lon="10"/></trkseg></trk></gpx>'
        path = tmp_path / 'padded.gpx'
        path.write_bytes(doc + b'\x00' * 16)
        with pytest.raises(fastgpx.ParseError, match='NUL byte'):
            fastgpx.load(path)

    def test_load_utf16_file(self, tmp_path: Path):
        # UTF-16 has a NUL byte in every ASCII character; those are left to the XML parser's
        # encoding detection rather than rejected.
        doc = '<gpx><trk><trkseg><trkpt lat="60" lon="10"/></trkseg></trk></gpx>'
        path = tmp_path / 'utf16.gpx'
        path.write_bytes(doc.encode('utf-16'))
        gpx = fastgpx.load(path)
        assert len(gpx.tracks[0].segments[0].points) == 1

    @pytest.mark.parametrize('time, microsecond', [
        ('2024-05-18T07:50:00.5Z', 500000),
        ('2024-05-18T07:50:00.123456Z', 123456),
        ('2024-05-18T07:50:00.1234567Z', 123456),
    ])
    def test_time_with_other_fraction_widths(self, time: str, microsecond: int):
        # Three fractional digits is the norm, but the format allows any number: one digit and
        # seven digits (.NET's round-trip format) both occur in gpxpy's test corpus.
        gpx = fastgpx.parse('<gpx><trk><trkseg><trkpt lat="60" lon="10">'
                            f'<time>{time}</time></trkpt></trkseg></trk></gpx>')
        assert gpx.time_bounds().start_time == datetime.datetime(
            2024, 5, 18, 7, 50, 0, microsecond, tzinfo=datetime.timezone.utc)

    @pytest.mark.parametrize('time, message', [
        ('2024-05-18T07:50:00.Z', 'expected fractional second digits'),
        ('2024-05-18T07:50:00Zx', 'unexpected characters after the time'),
    ])
    def test_malformed_time_raises_parse_error(self, time: str, message: str):
        gpx = fastgpx.parse('<gpx><trk><trkseg><trkpt lat="60" lon="10">'
                            f'<time>{time}</time></trkpt></trkseg></trk></gpx>')
        with pytest.raises(fastgpx.ParseError, match=message):
            gpx.time_bounds()

    def test_invalid_time_raises_parse_error_lazily(self):
        # Timestamps are parsed on demand, so the error surfaces from time_bounds(), not parse().
        gpx = fastgpx.parse('<gpx><trk><trkseg><trkpt lat="60" lon="10">'
                            '<time>not a time</time></trkpt></trkseg></trk></gpx>')
        with pytest.raises(fastgpx.ParseError):
            gpx.time_bounds()

    def test_encode_invalid_coordinate_is_fastgpx_error(self):
        with pytest.raises(fastgpx.Error, match='latitude out of range'):
            fastgpx.polyline.encode([fastgpx.LatLong(float('nan'), 0.0)])


class TestThreading:

    # `load` and `parse` release the GIL while parsing, so a thread pool must give the same
    # results as a sequential loop, and errors raised while the GIL was released must reach the
    # caller through the future.

    FILES = [
        'gpx/test/two-points.gpx',
        'gpx/test/debug-segment.gpx',
        'gpx/2024 TopCamp/Connected_20240518_094959_.gpx',
        'gpx/2024 Great Roadtrip/Connected_20240731_113605_Näsåker_Sollefteå.gpx',
    ]

    def test_load_from_thread_pool_matches_sequential(self):
        files = self.FILES * 4
        expected = [fastgpx.load(f).length_3d() for f in files]
        with ThreadPoolExecutor(max_workers=4) as pool:
            results = list(pool.map(lambda f: fastgpx.load(f).length_3d(), files))
        assert results == expected

    def test_parse_from_thread_pool_matches_sequential(self):
        data = [Path(f).read_text(encoding='utf-8') for f in self.FILES] * 4
        expected = [fastgpx.parse(d).length_3d() for d in data]
        with ThreadPoolExecutor(max_workers=4) as pool:
            results = list(pool.map(lambda d: fastgpx.parse(d).length_3d(), data))
        assert results == expected

    def test_errors_propagate_from_threads(self):
        with ThreadPoolExecutor(max_workers=2) as pool:
            parse_future = pool.submit(fastgpx.parse, 'not gpx')
            load_future = pool.submit(fastgpx.load, 'gpx/not-a-real-path/fake.gpx')
        with pytest.raises(fastgpx.ParseError):
            parse_future.result()
        with pytest.raises(FileNotFoundError):
            load_future.result()
