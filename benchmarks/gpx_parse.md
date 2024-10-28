# Gpx.Parse

## Before Adding TimeBounds

```sh
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

## Implementation 1: Always Parse Date

```sh
(.venv) C:\Users\Thomas\SourceTree\route-map>build\cpp\RelWithDebInfo\fastgpx_test.exe [parse][!benchmark]
Filters: [parse] [!benchmark]
Randomness seeded to: 1722472455

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
fastgpx_test.exe is a Catch2 v3.7.1 host application.
Run with -? for options

-------------------------------------------------------------------------------
Benchmark GPX Parsing
-------------------------------------------------------------------------------
C:\Users\Thomas\SourceTree\route-map\cpp\fastgpx_test.cpp(135)
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Connected_20240518_094959_.gpx                 100             1     3.08486 s
                                         29.668 ms    29.2905 ms    30.2068 ms
                                        2.27089 ms    1.73061 ms    3.23161 ms

Connected_20240520_103549_Lagerber-
gsgatan_35_45131_Uddevalla_Sweden.
gpx                                            100             1     4.58868 s
                                        39.6685 ms    39.3545 ms    40.2004 ms
                                        2.02734 ms    1.40523 ms    3.18799 ms


===============================================================================
test cases: 1 | 1 passed
assertions: - none -
```

## Implementation 2: Parse Date on Read

```sh
(.venv) C:\Users\Thomas\SourceTree\route-map>build\cpp\RelWithDebInfo\fastgpx_test.exe [parse][!benchmark]
Filters: [parse] [!benchmark]
Randomness seeded to: 284400238

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
fastgpx_test.exe is a Catch2 v3.7.1 host application.
Run with -? for options

-------------------------------------------------------------------------------
Benchmark GPX Parsing
-------------------------------------------------------------------------------
C:\Users\Thomas\SourceTree\route-map\cpp\fastgpx_test.cpp(135)
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Connected_20240518_094959_.gpx                 100             1     1.19506 s
                                        9.59077 ms     9.2823 ms    9.96809 ms
                                        1.73485 ms    1.46711 ms    2.20316 ms

Connected_20240520_103549_Lagerber-
gsgatan_35_45131_Uddevalla_Sweden.
gpx                                            100             1      1.1296 s
                                        11.1706 ms    11.0063 ms    11.4542 ms
                                        1.07504 ms    746.608 us    1.74628 ms


===============================================================================
test cases: 1 | 1 passed
assertions: - none -
```
