import datetime

import pytest

import fastgpx


UTC = datetime.timezone.utc


class TestLatLong:

    # fastgpx.LatLong.__init__

    def test_init_with_two_floats_positionals(self):
        latlong = fastgpx.LatLong(12.34, 56.78)
        assert latlong.latitude == 12.34
        assert latlong.longitude == 56.78
        assert latlong.elevation == 0.0

    def test_init_with_three_floats_positionals(self):
        latlong = fastgpx.LatLong(12.345678, 56.789012, 89.2)
        assert latlong.latitude == 12.345678
        assert latlong.longitude == 56.789012
        assert latlong.elevation == 89.2

    def test_init_with_two_integers_positionals(self):
        latlong = fastgpx.LatLong(12, 56)
        assert latlong.latitude == 12.0
        assert latlong.longitude == 56.0
        assert latlong.elevation == 0.0

    def test_init_with_three_integers_positionals(self):
        latlong = fastgpx.LatLong(12, 56, 89)
        assert latlong.latitude == 12.0
        assert latlong.longitude == 56.0
        assert latlong.elevation == 89.0

    def test_init_with_two_floats_keywords(self):
        latlong = fastgpx.LatLong(latitude=12.34, longitude=56.78)
        assert latlong.latitude == 12.34
        assert latlong.longitude == 56.78
        assert latlong.elevation == 0.0

    def test_init_with_three_floats_keywords(self):
        latlong = fastgpx.LatLong(latitude=12.345678, longitude=56.789012, elevation=89.2)
        assert latlong.latitude == 12.345678
        assert latlong.longitude == 56.789012
        assert latlong.elevation == 89.2

    def test_init_with_two_integers_keywords(self):
        latlong = fastgpx.LatLong(latitude=12, longitude=56)
        assert latlong.latitude == 12.0
        assert latlong.longitude == 56.0
        assert latlong.elevation == 0.0

    def test_init_with_three_integers_keywords(self):
        latlong = fastgpx.LatLong(latitude=12, longitude=56, elevation=89)
        assert latlong.latitude == 12.0
        assert latlong.longitude == 56.0
        assert latlong.elevation == 89.0

    def test_init_with_time_positional(self):
        time = datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)
        latlong = fastgpx.LatLong(12.34, 56.78, 89.2, time)
        assert latlong.time == time

    def test_init_with_time_keyword(self):
        time = datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)
        latlong = fastgpx.LatLong(latitude=12.34, longitude=56.78, time=time)
        assert latlong.time == time

    def test_init_without_time_leaves_it_none(self):
        assert fastgpx.LatLong(12.34, 56.78).time is None

    # fastgpx.LatLong.time

    def test_time_is_writable(self):
        time = datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)
        latlong = fastgpx.LatLong(12.34, 56.78)
        latlong.time = time
        assert latlong.time == time

    def test_time_can_be_cleared(self):
        time = datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)
        latlong = fastgpx.LatLong(12.34, 56.78, 0.0, time)
        latlong.time = None
        assert latlong.time is None

    def test_time_of_a_parsed_point(self):
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><time>2024-05-18T07:50:01Z</time></trkpt>'
               '<trkpt lat="60.6" lon="10.6"/>'
               '</trkseg></trk></gpx>')
        points = fastgpx.parse(xml).tracks[0].segments[0].points
        assert points[0].time == datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)
        assert points[1].time is None

    def test_time_of_a_malformed_timestamp_raises_when_read(self):
        # The <time> text is not parsed until it is read, so this is the first chance to report it.
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><time>not a time</time></trkpt>'
               '</trkseg></trk></gpx>')
        point = fastgpx.parse(xml).tracks[0].segments[0].points[0]
        with pytest.raises(fastgpx.ParseError):
            _ = point.time

    def test_time_keeps_sub_second_precision(self):
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><time>2024-05-18T07:50:01.123456Z</time></trkpt>'
               '</trkseg></trk></gpx>')
        point = fastgpx.parse(xml).tracks[0].segments[0].points[0]
        assert point.time == datetime.datetime(2024, 5, 18, 7, 50, 1, 123456, tzinfo=UTC)

    def test_time_of_a_naive_datetime_is_read_as_utc(self):
        # A datetime without a timezone is taken to be UTC, and reading it back gives an aware one,
        # so the value that comes out does not compare equal to the naive one that went in.
        naive = datetime.datetime(2024, 5, 18, 7, 50, 1)
        latlong = fastgpx.LatLong(12.34, 56.78, 0.0, naive)
        assert latlong.time == datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)
        assert latlong.time != naive

    def test_time_of_a_non_utc_datetime_is_converted(self):
        offset = datetime.timezone(datetime.timedelta(hours=2))
        latlong = fastgpx.LatLong(12.34, 56.78, 0.0,
                                  datetime.datetime(2024, 5, 18, 9, 50, 1, tzinfo=offset))
        assert latlong.time == datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)

    @pytest.mark.parametrize('value', [
        datetime.date(2024, 5, 18),
        datetime.time(7, 50, 1),
        datetime.time(7, 50, 1, tzinfo=UTC),
        '2024-05-18T07:50:01Z',
        1716018601,
    ])
    def test_time_rejects_anything_but_a_datetime(self, value):
        # A date or a time of day is not a point in time, and the stock conversion would silently
        # make one up (midnight, or 1970-01-01).
        latlong = fastgpx.LatLong(12.34, 56.78)
        with pytest.raises(TypeError):
            latlong.time = value
        assert latlong.time is None
        with pytest.raises(TypeError):
            fastgpx.LatLong(12.34, 56.78, 0.0, value)
        with pytest.raises(TypeError):
            fastgpx.LatLong(12.34, 56.78, time=value)

    def test_time_accepts_a_datetime_subclass(self):
        class MyDateTime(datetime.datetime):
            pass

        time = MyDateTime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)
        assert fastgpx.LatLong(12.34, 56.78, 0.0, time).time == time

    @pytest.mark.parametrize('time', [
        '2024-05-18T07:50:01Z',
        '2024-05-18T07:50:01.123Z',
        '2024-05-18T09:50:01.123456+02:00',
        # Longer than the text a point stores inline, so it is kept on the heap.
        '2024-05-18T07:50:01.123456000000000000000000000Z',
    ])
    def test_copying_a_point_keeps_its_time(self, time: str):
        xml = ('<gpx><trk><trkseg>'
               f'<trkpt lat="60.5" lon="10.5"><time>{time}</time></trkpt>'
               f'<trkpt lat="60.6" lon="10.6"><time>{time}</time></trkpt>'
               '</trkseg></trk></gpx>')
        points = fastgpx.parse(xml).tracks[0].segments[0].points
        expected = points[0].time

        copies = list(points)
        del points
        assert [copy.time for copy in copies] == [expected, expected]
        assert [copy.time for copy in list(copies)] == [expected, expected]

    # fastgpx.LatLong.__eq__

    def test_equality(self):
        latlong1 = fastgpx.LatLong(12.34, 56.78, 90.1)
        latlong2 = fastgpx.LatLong(12.34, 56.78, 90.1)
        latlong3 = fastgpx.LatLong(12.34, 56.78, 91.2)

        assert latlong1 == latlong2
        assert latlong1 != latlong3

    def test_equality_includes_the_time(self):
        time = datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)
        later = datetime.datetime(2024, 5, 18, 7, 50, 2, tzinfo=UTC)

        assert fastgpx.LatLong(12.34, 56.78, 0.0, time) == fastgpx.LatLong(12.34, 56.78, 0.0, time)
        assert fastgpx.LatLong(12.34, 56.78, 0.0, time) != fastgpx.LatLong(12.34, 56.78, 0.0, later)
        assert fastgpx.LatLong(12.34, 56.78, 0.0, time) != fastgpx.LatLong(12.34, 56.78)

    def test_equality_of_a_parsed_point_and_a_constructed_one(self):
        # A parsed point holds the <time> as text and a constructed one holds a datetime. They name
        # the same instant, so they compare equal. See #16.
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><ele>100.0</ele>'
               '<time>2024-05-18T07:50:01Z</time></trkpt>'
               '</trkseg></trk></gpx>')
        point = fastgpx.parse(xml).tracks[0].segments[0].points[0]
        time = datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)
        assert point == fastgpx.LatLong(60.5, 10.5, 100.0, time)

    def test_equality_is_not_changed_by_reading_the_time(self):
        # Indexing `points` copies the point, so reading `time` on the copy parses that copy's text
        # and leaves the stored one as text. Both still name the same instant. See #16.
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><time>2024-05-18T07:50:01Z</time></trkpt>'
               '</trkseg></trk></gpx>')
        points = fastgpx.parse(xml).tracks[0].segments[0].points

        point = points[0]
        assert points[0] == point
        point.time  # Replaces this copy's text with the instant it parses to.
        assert points[0] == point

    def test_equality_of_a_malformed_timestamp_does_not_raise(self):
        # A timestamp neither side can parse compares by its text, so `==` stays usable on a
        # document fastgpx cannot fully read. See #16.
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><time>not a time</time></trkpt>'
               '</trkseg></trk></gpx>')
        first = fastgpx.parse(xml).tracks[0].segments[0].points[0]
        second = fastgpx.parse(xml).tracks[0].segments[0].points[0]
        third = fastgpx.parse(xml.replace('not a time', 'also not a time'))
        third_point = third.tracks[0].segments[0].points[0]

        assert first == second
        assert first != third_point
        assert first != fastgpx.LatLong(60.5, 10.5)

    def test_membership_of_a_constructed_point_in_the_parsed_points(self):
        # `in`, `index` and `count` compare against the points still held by the segment, which is
        # where the parsed and unparsed states meet most visibly. See #16.
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><time>2024-05-18T07:50:01Z</time></trkpt>'
               '<trkpt lat="60.6" lon="10.6"><time>2024-05-18T07:50:02Z</time></trkpt>'
               '</trkseg></trk></gpx>')
        points = fastgpx.parse(xml).tracks[0].segments[0].points
        second = fastgpx.LatLong(60.6, 10.6, 0.0,
                                 datetime.datetime(2024, 5, 18, 7, 50, 2, tzinfo=UTC))

        assert second in points
        assert points.index(second) == 1
        assert points.count(second) == 1

    def test_equality_is_at_microsecond_resolution(self):
        # `time` is a datetime, which holds microseconds, so equality stops there too, and does so
        # on every platform whatever the resolution of the C++ clock.
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><time>2024-05-18T07:50:01.0000001Z</time></trkpt>'
               '<trkpt lat="60.5" lon="10.5"><time>2024-05-18T07:50:01.0000009Z</time></trkpt>'
               '<trkpt lat="60.5" lon="10.5"><time>2024-05-18T07:50:01.000001Z</time></trkpt>'
               '</trkseg></trk></gpx>')
        points = fastgpx.parse(xml).tracks[0].segments[0].points
        assert points[0] == points[1]
        assert points[0] != points[2]

    @pytest.mark.parametrize('time', [
        '2024-05-18T07:50:01Z',
        '2024-05-18T07:50:01.1234567Z',
        '2024-05-18T07:50:01.9999999Z',
        '1969-12-31T23:59:59.9999999Z',
        '2024-05-18T09:50:01.1234567+02:00',
    ])
    def test_a_point_rebuilt_from_its_time_is_equal(self, time: str):
        xml = ('<gpx><trk><trkseg>'
               f'<trkpt lat="60.5" lon="10.5"><ele>100.0</ele><time>{time}</time></trkpt>'
               '</trkseg></trk></gpx>')
        point = fastgpx.parse(xml).tracks[0].segments[0].points[0]
        rebuilt = fastgpx.LatLong(point.latitude, point.longitude, point.elevation, point.time)
        assert rebuilt == point
        assert hash(rebuilt) == hash(point)

    # fastgpx.LatLong.__hash__

    def test_hash_of_a_parsed_and_a_constructed_point(self):
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><ele>100.0</ele>'
               '<time>2024-05-18T09:50:01.0000009+02:00</time></trkpt>'
               '</trkseg></trk></gpx>')
        p = fastgpx.parse(xml).tracks[0].segments[0].points[0]
        q = fastgpx.LatLong(60.5, 10.5, 100.0, datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC))
        assert p == q
        assert hash(p) == hash(q)
        assert len({p, q}) == 1
        assert {p: 'parsed'}[q] == 'parsed'

    def test_hash_does_not_change_when_the_time_is_read(self):
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><time>2024-05-18T07:50:01Z</time></trkpt>'
               '</trkseg></trk></gpx>')
        point = fastgpx.parse(xml).tracks[0].segments[0].points[0]
        before = hash(point)
        point.time  # Replaces the text with the instant it parses to.
        assert hash(point) == before

    def test_hash_of_points_without_a_time(self):
        assert len({fastgpx.LatLong(60.5, 10.5), fastgpx.LatLong(60.5, 10.5, 0.0)}) == 1
        assert len({fastgpx.LatLong(60.5, 10.5), fastgpx.LatLong(60.5, 10.5, 1.0)}) == 2

    def test_hash_of_negative_zero(self):
        # -0.0 == 0.0, so they must hash alike.
        p = fastgpx.LatLong(0.0, -0.0, -0.0)
        q = fastgpx.LatLong(-0.0, 0.0, 0.0)
        assert p == q
        assert len({p, q}) == 1

    def test_hash_of_nan_is_stable(self):
        # NaN equals nothing, not even itself, but the hash of one object must not change.
        point = fastgpx.LatLong(float('nan'), 10.5)
        assert point != point
        assert hash(point) == hash(point)
        assert point in {point}  # Found by identity.

    def test_hash_of_a_malformed_timestamp(self):
        # A timestamp that does not parse compares by its text, and hashes by it.
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><time>not a time</time></trkpt>'
               '</trkseg></trk></gpx>')
        first = fastgpx.parse(xml).tracks[0].segments[0].points[0]
        second = fastgpx.parse(xml).tracks[0].segments[0].points[0]
        other = fastgpx.parse(xml.replace('not a time', 'also not a time'))
        assert len({first, second}) == 1
        assert len({first, other.tracks[0].segments[0].points[0]}) == 2

    def test_hash_distinguishes_the_time(self):
        time = datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)
        points = {
            fastgpx.LatLong(60.5, 10.5, 0.0, time),
            fastgpx.LatLong(60.5, 10.5, 0.0, time + datetime.timedelta(microseconds=1)),
            fastgpx.LatLong(60.5, 10.5, 0.0),
        }
        assert len(points) == 3

    # fastgpx.LatLong.__repr__

    def test_repr(self):
        latlong = fastgpx.LatLong(12.34, 56.78, 89.2)
        repr_str = repr(latlong)
        assert repr_str == ('fastgpx.LatLong(latitude=12.34, longitude=56.78, elevation=89.2, '
                            'time=None)')

    def test_repr_with_a_time(self):
        time = datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)
        latlong = fastgpx.LatLong(12.34, 56.78, 89.2, time)
        assert repr(latlong) == (
            'fastgpx.LatLong(latitude=12.34, longitude=56.78, elevation=89.2, '
            'time=datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=datetime.timezone.utc))')

    def test_repr_of_a_malformed_timestamp_shows_the_text(self):
        # Raising here would make a point with a malformed <time> impossible to print.
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><time>not a time</time></trkpt>'
               '</trkseg></trk></gpx>')
        point = fastgpx.parse(xml).tracks[0].segments[0].points[0]
        assert repr(point) == ('fastgpx.LatLong(latitude=60.5, longitude=10.5, elevation=0, '
                               "time='not a time')")

    # fastgpx.LatLong.__str__

    def test_str(self):
        latlong = fastgpx.LatLong(12.34, 56.78, 89.2)
        str_str = str(latlong)
        assert str_str == 'LatLong(12.34, 56.78, 89.2, None)'

    def test_str_with_a_time(self):
        time = datetime.datetime(2024, 5, 18, 7, 50, 1, tzinfo=UTC)
        latlong = fastgpx.LatLong(12.34, 56.78, 89.2, time)
        assert str(latlong) == 'LatLong(12.34, 56.78, 89.2, 2024-05-18T07:50:01Z)'

    def test_str_of_a_malformed_timestamp_shows_the_text(self):
        xml = ('<gpx><trk><trkseg>'
               '<trkpt lat="60.5" lon="10.5"><time>not a time</time></trkpt>'
               '</trkseg></trk></gpx>')
        point = fastgpx.parse(xml).tracks[0].segments[0].points[0]
        assert str(point) == "LatLong(60.5, 10.5, 0, 'not a time')"
