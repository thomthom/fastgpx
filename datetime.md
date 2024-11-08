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


```cpp
#include <print>
#include <chrono>
#include <ctime>
#include <string>


int main() {
    std::println("Hello {}", 123);

    // 2024-11-08T15:32:22.440Z
    // 1731079942
    std::time_t unix_timestamp = 1731079942;
    const std::string time_tr = "2024-11-08T15:32:22.440Z";

    const auto sys_time1 = std::chrono::sys_seconds{std::chrono::seconds{unix_timestamp}};
    // Making `utc_time` const affect output... ???
    auto utc_time1 = std::chrono::clock_cast<std::chrono::utc_clock>(sys_time1);
    std::println("UTC Time: {}", utc_time1);

    std::tm cal;
    cal.tm_year = 2024 - 1900;
    cal.tm_mon = 11 - 1; // Zero based month
    cal.tm_mday = 8;
    cal.tm_hour = 15;
    cal.tm_min = 32;
    cal.tm_sec = 22;
    const auto cal_time = std::mktime(&cal);
    const auto sys_time2 = std::chrono::sys_seconds{std::chrono::seconds{cal_time}};
    const auto utc_time2 = std::chrono::clock_cast<std::chrono::utc_clock>(sys_time2);
    std::println("UTC Time: {}", utc_time2);

    // const auto msecs = std::chrono::microseconds(440);
    const auto msecs = std::chrono::milliseconds(440);
    auto sys_time_ms = sys_time2 + msecs;
    auto utc_time_ms = std::chrono::clock_cast<std::chrono::utc_clock>(sys_time_ms);
    std::println(" UTC Time: {}", utc_time_ms);

    std::println("  ISO8601: {}", time_tr);

    std::println("     UNIX: {}", unix_timestamp);
    std::println("(ms) UNIX: {}", 1731079942440);
    std::println("utc_clock: {}", utc_time_ms.time_since_epoch().count());
}
```

x86-64 clang (trunk) `-std=c++23`
```
ASM generation compiler returned: 0
Execution build compiler returned: 0
Program returned: 0
Hello 123
UTC Time: 2024-11-08 15:32:22
UTC Time: 2024-11-08 15:32:22
 UTC Time: 2024-11-08 15:32:22.440
  ISO8601: 2024-11-08T15:32:22.440Z
     UNIX: 1731079942
(ms) UNIX: 1731079942440
utc_clock: 1731079969440
```

TODO: Modify `python_utc_chrono.hpp` to cast `std::chrono::utc_clock`.
