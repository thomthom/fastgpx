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
