# Gpx.Parse

## Before Adding TimeBounds

```sh
(.venv) C:\Users\Thomas\SourceTree\route-map>build\cpp\RelWithDebInfo\fastgpx_test.exe [parse][!benchmark]
Filters: [parse] [!benchmark]
Randomness seeded to: 774260448

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
fastgpx_test.exe is a Catch2 v3.7.1 host application.
Run with -? for options

-------------------------------------------------------------------------------
Benchmark GPX Parsing
-------------------------------------------------------------------------------
C:\Users\Thomas\SourceTree\route-map\cpp\fastgpx_test.cpp(87)
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Connected_20240518_094959_.gpx                 100             1    827.424 ms
                                        6.63084 ms    6.44582 ms    6.83688 ms
                                        994.442 us    909.943 us    1.07019 ms

Connected_20240520_103549_Lagerber-
gsgatan_35_45131_Uddevalla_Sweden.
gpx                                            100             1    876.149 ms
                                         8.8731 ms    8.76913 ms    9.03321 ms
                                        649.172 us    477.529 us    961.938 us


===============================================================================
test cases: 1 | 1 passed
assertions: - none -


(.venv) C:\Users\Thomas\SourceTree\route-map>build\cpp\RelWithDebInfo\fastgpx_test.exe [parse][!benchmark]
Filters: [parse] [!benchmark]
Randomness seeded to: 3537335085

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
fastgpx_test.exe is a Catch2 v3.7.1 host application.
Run with -? for options

-------------------------------------------------------------------------------
Benchmark GPX Parsing
-------------------------------------------------------------------------------
C:\Users\Thomas\SourceTree\route-map\cpp\fastgpx_test.cpp(87)
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Connected_20240518_094959_.gpx                 100             1    835.242 ms
                                        7.15956 ms    6.97966 ms    7.36515 ms
                                        978.563 us    868.732 us    1.08853 ms

Connected_20240520_103549_Lagerber-
gsgatan_35_45131_Uddevalla_Sweden.
gpx                                            100             1    907.622 ms
                                        9.21123 ms    9.02814 ms    9.47385 ms
                                        1.10728 ms    833.934 us    1.51319 ms


===============================================================================
test cases: 1 | 1 passed
assertions: - none -


(.venv) C:\Users\Thomas\SourceTree\route-map>build\cpp\RelWithDebInfo\fastgpx_test.exe [parse][!benchmark]
Filters: [parse] [!benchmark]
Randomness seeded to: 909097062

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
fastgpx_test.exe is a Catch2 v3.7.1 host application.
Run with -? for options

-------------------------------------------------------------------------------
Benchmark GPX Parsing
-------------------------------------------------------------------------------
C:\Users\Thomas\SourceTree\route-map\cpp\fastgpx_test.cpp(87)
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Connected_20240518_094959_.gpx                 100             1    833.013 ms
                                        6.96422 ms    6.81391 ms    7.13573 ms
                                        820.892 us    731.942 us    934.469 us

Connected_20240520_103549_Lagerber-
gsgatan_35_45131_Uddevalla_Sweden.
gpx                                            100             1    879.365 ms
                                        9.09393 ms    8.97689 ms    9.35026 ms
                                        841.074 us    390.829 us    1.42496 ms


===============================================================================
test cases: 1 | 1 passed
assertions: - none -
```

---

## Implementation 1

```sh
(.venv) C:\Users\Thomas\SourceTree\route-map>build\cpp\RelWithDebInfo\fastgpx_test.exe [parse][!benchmark]
Filters: [parse] [!benchmark]
Randomness seeded to: 2961048210

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
fastgpx_test.exe is a Catch2 v3.7.1 host application.
Run with -? for options

-------------------------------------------------------------------------------
Benchmark GPX Parsing
-------------------------------------------------------------------------------
C:\Users\Thomas\SourceTree\route-map\cpp\fastgpx_test.cpp(100)
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Connected_20240518_094959_.gpx                 100             1     3.10751 s
                                        30.3941 ms      30.12 ms    30.7201 ms
                                        1.51718 ms    1.25833 ms     1.9593 ms

Connected_20240520_103549_Lagerber-
gsgatan_35_45131_Uddevalla_Sweden.
gpx                                            100             1     4.11059 s
                                        43.2513 ms    42.5824 ms    44.0343 ms
                                        3.66523 ms    3.18018 ms    4.28064 ms


===============================================================================
test cases: 1 | 1 passed
assertions: - none -


(.venv) C:\Users\Thomas\SourceTree\route-map>build\cpp\RelWithDebInfo\fastgpx_test.exe [parse][!benchmark]
Filters: [parse] [!benchmark]
Randomness seeded to: 2429539549

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
fastgpx_test.exe is a Catch2 v3.7.1 host application.
Run with -? for options

-------------------------------------------------------------------------------
Benchmark GPX Parsing
-------------------------------------------------------------------------------
C:\Users\Thomas\SourceTree\route-map\cpp\fastgpx_test.cpp(100)
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Connected_20240518_094959_.gpx                 100             1     3.11613 s
                                        31.1314 ms    30.8928 ms    31.5305 ms
                                        1.54351 ms    1.06384 ms    2.32682 ms

Connected_20240520_103549_Lagerber-
gsgatan_35_45131_Uddevalla_Sweden.
gpx                                            100             1      4.1663 s
                                        42.0397 ms     41.818 ms    42.3478 ms
                                         1.3144 ms    1.00605 ms    2.02744 ms


===============================================================================
test cases: 1 | 1 passed
assertions: - none -


(.venv) C:\Users\Thomas\SourceTree\route-map>build\cpp\RelWithDebInfo\fastgpx_test.exe [parse][!benchmark]
Filters: [parse] [!benchmark]
Randomness seeded to: 4265016528

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
fastgpx_test.exe is a Catch2 v3.7.1 host application.
Run with -? for options

-------------------------------------------------------------------------------
Benchmark GPX Parsing
-------------------------------------------------------------------------------
C:\Users\Thomas\SourceTree\route-map\cpp\fastgpx_test.cpp(100)
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Connected_20240518_094959_.gpx                 100             1     3.06944 s
                                        30.1539 ms    29.8875 ms    30.7957 ms
                                        1.99009 ms    986.131 us    4.06382 ms

Connected_20240520_103549_Lagerber-
gsgatan_35_45131_Uddevalla_Sweden.
gpx                                            100             1     4.29093 s
                                        40.8043 ms    40.5662 ms    41.1527 ms
                                        1.45447 ms    1.07231 ms    1.94029 ms


===============================================================================
test cases: 1 | 1 passed
assertions: - none -
```
