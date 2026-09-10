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
