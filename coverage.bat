OpenCppCoverage.exe^
  --export_type html:coverage^
  --modules fastgpx_test.exe^
  --sources src\cpp\fastgpx\*^
  --excluded_sources src\cpp\fastgpx\*_test.cpp^
  --excluded_sources src\cpp\fastgpx\test_data.*^
  --^
  build\src\cpp\Debug\fastgpx_test.exe ~[!benchmark] ~[!mayfail] %*
