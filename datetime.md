tests\test_bounds.py ....                                                [ 20%]
tests\test_fastgpx.py .....F                                             [ 50%]
tests\test_polyline.py ..........                                        [100%]

================================== FAILURES ===================================
____________________________ test_gpx_time_bounds _____________________________

gpx_path = 'gpx/2024 TopCamp/Connected_20240518_094959_.gpx'

    def test_gpx_time_bounds(gpx_path: str):
        gpx = fastgpx.parse(gpx_path)
        time_bounds = gpx.time_bounds()
        assert not time_bounds.is_empty()

        # 2024-05-18T07:50:00Z
        assert time_bounds.start_time is not None
>       assert time_bounds.start_time == datetime.datetime(
            year=2024, month=5, day=18, hour=7, minute=50, second=0)
E       AssertionError: assert datetime.datetime(2024, 5, 18, 1, 50) == datetime.datetime(2024, 5, 18, 7, 50)
E        +  where datetime.datetime(2024, 5, 18, 1, 50) = <fastgpx.TimeBounds object at 0x0000013592C1FC70>.start_time
E        +  and   datetime.datetime(2024, 5, 18, 7, 50) = <class 'datetime.datetime'>(year=2024, month=5, day=18, hour=7, minute=50, second=0)
E        +    where <class 'datetime.datetime'> = datetime.datetime

tests\test_fastgpx.py:80: AssertionError
=========================== short test summary info ===========================
FAILED tests/test_fastgpx.py::test_gpx_time_bounds - AssertionError: assert d...
======================== 1 failed, 19 passed in 0.60s =========================
Finished running tests!
