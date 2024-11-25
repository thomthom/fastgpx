OpenCppCoverage.exe^
  --export_type html:coverage^
  --sources cpp\fastgpx\*^
  --excluded_sources cpp\fastgpx\*_test.cpp^
  --excluded_sources cpp\fastgpx\test_data.*^
  --^
  build\cpp\Debug\fastgpx_test.exe ~[!benchmark] ~[!mayfail] [gpxtime]
