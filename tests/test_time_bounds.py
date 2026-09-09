import sys
from datetime import datetime, timedelta, timezone

import pytest

import fastgpx


class TestTimeBounds:

    # fastgpx.TimeBounds.__init__

    def test_init_defaults(self):
        bounds = fastgpx.TimeBounds()
        assert bounds.is_empty()
        assert not bounds.is_range()
        assert bounds.start_time is None
        assert bounds.end_time is None

    def test_init_with_start_time(self):
        start_time = datetime.fromisoformat("2025-06-20 08:07:28+00:00")
        bounds = fastgpx.TimeBounds(start_time=start_time, end_time=None)
        assert not bounds.is_empty()
        assert not bounds.is_range()
        assert bounds.start_time == start_time
        assert bounds.end_time is None

    def test_init_with_end_time(self):
        end_time = datetime.fromisoformat("2025-06-27 13:37:00+00:00")
        bounds = fastgpx.TimeBounds(start_time=None, end_time=end_time)
        assert not bounds.is_empty()
        assert not bounds.is_range()
        assert bounds.start_time is None
        assert bounds.end_time == end_time

    def test_init_with_start_and_end_time(self):
        start_time = datetime.fromisoformat("2025-06-20 08:07:28+00:00")
        end_time = datetime.fromisoformat("2025-06-27 13:37:00+00:00")
        bounds = fastgpx.TimeBounds(start_time=start_time, end_time=end_time)
        assert not bounds.is_empty()
        assert bounds.is_range()
        assert bounds.start_time == start_time
        assert bounds.end_time == end_time

    # fastgpx.TimeBounds.add

    def test_add_single_time_point(self):
        bounds = fastgpx.TimeBounds()
        time_point = datetime.fromisoformat("2025-06-20 08:07:28+00:00")
        bounds.add(time_point)
        assert not bounds.is_empty()
        assert bounds.is_range()
        assert bounds.start_time == time_point
        assert bounds.end_time == time_point

    def test_add_second_time_point(self):
        bounds = fastgpx.TimeBounds()
        time_point1 = datetime.fromisoformat("2025-06-20 08:07:28+00:00")
        time_point2 = datetime.fromisoformat("2025-06-27 13:37:00+00:00")
        bounds.add(time_point1)
        bounds.add(time_point2)
        assert not bounds.is_empty()
        assert bounds.is_range()
        assert bounds.start_time == time_point1
        assert bounds.end_time == time_point2

    def test_add_timezone_aware_time_point(self):
        bounds = fastgpx.TimeBounds()
        # 12:00 at UTC+02:00 is 10:00 UTC.
        time_point = datetime(2025, 6, 20, 12, 0, 0, tzinfo=timezone(timedelta(hours=2)))
        bounds.add(time_point)
        assert bounds.start_time == time_point
        assert bounds.start_time == datetime(2025, 6, 20, 10, 0, 0, tzinfo=timezone.utc)
        assert bounds.start_time.tzinfo is timezone.utc

    def test_add_naive_time_point_is_utc(self):
        bounds = fastgpx.TimeBounds()
        bounds.add(datetime(2025, 6, 20, 12, 0, 0))
        assert bounds.start_time == datetime(2025, 6, 20, 12, 0, 0, tzinfo=timezone.utc)

    def test_add_invalid_utcoffset_raises_type_error(self):
        # A datetime subclass whose `utcoffset()` returns something other than a timedelta must
        # be rejected as an incompatible argument, not crash the process.
        class BadOffsetDateTime(datetime):
            def utcoffset(self):
                return 3600

        bounds = fastgpx.TimeBounds()
        with pytest.raises(TypeError, match='incompatible function arguments'):
            bounds.add(BadOffsetDateTime(2025, 6, 20, 12, 0, 0, tzinfo=timezone.utc))
        assert bounds.is_empty()

    def test_add_naive_time_point_ignores_utcoffset_override(self):
        # For naive datetimes `tzinfo` is None, so `utcoffset()` is never consulted.
        class BadOffsetDateTime(datetime):
            def utcoffset(self):
                return 3600

        bounds = fastgpx.TimeBounds()
        bounds.add(BadOffsetDateTime(2025, 6, 20, 12, 0, 0))
        assert bounds.start_time == datetime(2025, 6, 20, 12, 0, 0, tzinfo=timezone.utc)

    def test_add_time_point_with_microseconds(self):
        bounds = fastgpx.TimeBounds()
        time_point = datetime(2025, 6, 20, 12, 0, 0, 123456, tzinfo=timezone.utc)
        bounds.add(time_point)
        assert bounds.start_time == time_point
        assert bounds.start_time.microsecond == 123456

    def test_add_time_point_before_epoch(self):
        # Negative time since the epoch: the time of day and the sub-second part must still come
        # out non-negative. (Before the C++20 calendar conversion, Windows raised ValueError for
        # any time point before 1970.)
        bounds = fastgpx.TimeBounds()
        time_point = datetime(1969, 12, 31, 23, 59, 59, 500000, tzinfo=timezone.utc)
        bounds.add(time_point)
        assert bounds.start_time == time_point

    @pytest.mark.parametrize('time_point', [
        datetime(1, 1, 1, tzinfo=timezone.utc),
        datetime(9999, 12, 31, 23, 59, 59, 999999, tzinfo=timezone.utc),
    ])
    def test_add_time_point_at_datetime_limits(self, time_point):
        # `system_clock` covers the whole `datetime` range on MSVC (100 ns ticks) but only about
        # 1677..2262 on libstdc++ (nanoseconds). Outside that range the value must be rejected as
        # an incompatible argument, never silently converted to a wrong time.
        bounds = fastgpx.TimeBounds()
        try:
            bounds.add(time_point)
        except TypeError as error:
            assert sys.platform != 'win32'
            assert 'incompatible function arguments' in str(error)
            assert bounds.is_empty()
        else:
            assert bounds.start_time == time_point

    def test_init_with_timezone_aware_times(self):
        start_time = datetime(2025, 6, 20, 8, 7, 28, tzinfo=timezone(timedelta(hours=-5)))
        end_time = datetime(2025, 6, 27, 13, 37, 0, tzinfo=timezone(timedelta(hours=5, minutes=30)))
        bounds = fastgpx.TimeBounds(start_time=start_time, end_time=end_time)
        assert bounds.start_time == start_time
        assert bounds.end_time == end_time
        assert bounds.start_time.isoformat() == '2025-06-20T13:07:28+00:00'
        assert bounds.end_time.isoformat() == '2025-06-27T08:07:00+00:00'

    def test_add_time_bounds(self):
        bounds1 = fastgpx.TimeBounds(
            start_time=datetime.fromisoformat("2025-06-20 08:07:28+00:00"),
            end_time=datetime.fromisoformat("2025-06-27 13:37:00+00:00")
        )
        bounds2 = fastgpx.TimeBounds(
            start_time=datetime.fromisoformat("2025-06-25 10:00:00+00:00"),
            end_time=datetime.fromisoformat("2025-06-30 15:00:00+00:00")
        )
        bounds1.add(bounds2)
        assert not bounds1.is_empty()
        assert bounds1.is_range()
        assert bounds1.start_time == bounds1.start_time
        assert bounds1.end_time == bounds2.end_time

    # fastgpx.TimeBounds.start_time

    def test_start_time_set_new_time(self):
        bounds = fastgpx.TimeBounds()
        new_start_time = datetime.fromisoformat("2025-06-20 08:07:28+00:00")
        bounds.start_time = new_start_time
        assert not bounds.is_empty()
        assert not bounds.is_range()
        assert bounds.start_time == new_start_time
        assert bounds.end_time is None

    def test_start_time_set_none(self):
        bounds = fastgpx.TimeBounds(
            start_time=datetime.fromisoformat("2025-06-20 08:07:28+00:00"),
            end_time=datetime.fromisoformat("2025-06-27 13:37:00+00:00")
        )
        bounds.start_time = None
        assert not bounds.is_empty()
        assert not bounds.is_range()
        assert bounds.start_time is None
        assert bounds.end_time == datetime.fromisoformat("2025-06-27 13:37:00+00:00")

    # fastgpx.TimeBounds.end_time

    def test_end_time_set_new_time(self):
        bounds = fastgpx.TimeBounds()
        new_end_time = datetime.fromisoformat("2025-06-27 13:37:00+00:00")
        bounds.end_time = new_end_time
        assert not bounds.is_empty()
        assert not bounds.is_range()
        assert bounds.start_time is None
        assert bounds.end_time == new_end_time

    def test_end_time_set_none(self):
        bounds = fastgpx.TimeBounds(
            start_time=datetime.fromisoformat("2025-06-20 08:07:28+00:00"),
            end_time=datetime.fromisoformat("2025-06-27 13:37:00+00:00")
        )
        bounds.end_time = None
        assert not bounds.is_empty()
        assert not bounds.is_range()
        assert bounds.start_time == datetime.fromisoformat("2025-06-20 08:07:28+00:00")
        assert bounds.end_time is None

    # fastgpx.TimeBounds.__eq__

    def test_equality(self):
        start_time = datetime.fromisoformat("2025-06-20 08:07:28+00:00")
        end_time = datetime.fromisoformat("2025-06-27 13:37:00+00:00")
        bounds1 = fastgpx.TimeBounds(start_time=start_time, end_time=end_time)
        bounds2 = fastgpx.TimeBounds(start_time=start_time, end_time=end_time)
        bounds3 = fastgpx.TimeBounds(start_time=start_time, end_time=None)

        assert bounds1 == bounds2
        assert bounds1 != bounds3

    # fastgpx.TimeBounds.__repr__

    def test_repr(self):
        start_time = datetime.fromisoformat("2025-06-20 08:07:28+00:00")
        end_time = datetime.fromisoformat("2025-06-27 13:37:00+00:00")
        bounds = fastgpx.TimeBounds(start_time=start_time, end_time=end_time)
        repr_str = repr(bounds)
        expected_str = (
            "fastgpx.TimeBounds(start_time=datetime.datetime(2025, 6, 20, 8, 7, 28, tzinfo=datetime.timezone.utc), "
            "end_time=datetime.datetime(2025, 6, 27, 13, 37, tzinfo=datetime.timezone.utc))"
        )
        assert repr_str == expected_str

    # fastgpx.TimeBounds.__str__

    def test_str(self):
        start_time = datetime.fromisoformat("2025-06-20 08:07:28+00:00")
        end_time = datetime.fromisoformat("2025-06-27 13:37:00+00:00")
        bounds = fastgpx.TimeBounds(start_time=start_time, end_time=end_time)
        str_str = str(bounds)
        expected_str = "TimeBounds(2025-06-20T08:07:28Z to 2025-06-27T13:37:00Z)"
        assert str_str == expected_str
