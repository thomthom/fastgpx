from datetime import datetime

import fastgpx


class TestTimeBounds:

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
