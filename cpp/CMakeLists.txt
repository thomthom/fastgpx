cmake_minimum_required(VERSION 3.30.2)

project(GpxReader)

include(FetchContent)

FetchContent_Declare(
  tinyxml2
  GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
  GIT_TAG        321ea883b7190d4e85cae5512a12e5eaa8f8731f # v10.0.0
)
FetchContent_MakeAvailable(tinyxml2)

add_executable(gpxreader main.cpp)
set_property(TARGET gpxreader PROPERTY CXX_STANDARD 23)
set_property(TARGET gpxreader PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET gpxreader PROPERTY CXX_EXTENSIONS OFF)
target_link_libraries(gpxreader tinyxml2)

