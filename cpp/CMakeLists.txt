include(FetchContent)

FetchContent_Declare(
  tinyxml2
  GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
  GIT_TAG        321ea883b7190d4e85cae5512a12e5eaa8f8731f # v10.0.0
)

FetchContent_Declare(
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG        db78afc2b7d8f043b4bc6b185635d949ea2ed2a8 # v1.14
)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.7.1
)

FetchContent_MakeAvailable(tinyxml2 pugixml Catch2)

add_executable(gpxreader main.cpp gpx.hpp gpx.cpp fastgpx.hpp fastgpx.cpp)
set_property(TARGET gpxreader PROPERTY CXX_STANDARD 23)
set_property(TARGET gpxreader PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET gpxreader PROPERTY CXX_EXTENSIONS OFF)
target_link_libraries(gpxreader PRIVATE tinyxml2 pugixml)

# fastgpx

find_package(Python3 3.11 EXACT REQUIRED COMPONENTS Interpreter Development)

set(PYBIND11_NEWPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(gpxcpp gpxcpp.cpp gpx.hpp gpx.cpp fastgpx.hpp fastgpx.cpp)
set_property(TARGET gpxcpp PROPERTY CXX_STANDARD 23)
set_property(TARGET gpxcpp PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET gpxcpp PROPERTY CXX_EXTENSIONS OFF)
target_link_libraries(gpxcpp PRIVATE tinyxml2 pugixml)

install(TARGETS gpxcpp LIBRARY DESTINATION .)

add_custom_command(TARGET gpxcpp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:gpxcpp>"  # This is the .pyd file
        "${CMAKE_SOURCE_DIR}/.venv/Lib/site-packages/gpxcpp.cp311-win_amd64.pyd"
)

# fastgpx static library

add_library(fastgpx-static fastgpx.hpp fastgpx.cpp)
set_property(TARGET fastgpx-static PROPERTY CXX_STANDARD 23)
set_property(TARGET fastgpx-static PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET fastgpx-static PROPERTY CXX_EXTENSIONS OFF)
target_link_libraries(fastgpx-static PUBLIC tinyxml2 pugixml)

# fastgpx python module

pybind11_add_module(fastgpx python_fastgpx.cpp)
set_property(TARGET fastgpx PROPERTY CXX_STANDARD 23)
set_property(TARGET fastgpx PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET fastgpx PROPERTY CXX_EXTENSIONS OFF)
target_link_libraries(fastgpx PRIVATE fastgpx-static)

install(TARGETS fastgpx LIBRARY DESTINATION .)

add_custom_command(TARGET fastgpx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:fastgpx>"  # This is the .pyd file
        "${CMAKE_SOURCE_DIR}/.venv/Lib/site-packages/fastgpx.cp311-win_amd64.pyd"
)

# fastgpx debug app

add_executable(fastgpxcli app.cpp)
set_property(TARGET fastgpxcli PROPERTY CXX_STANDARD 23)
set_property(TARGET fastgpxcli PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET fastgpxcli PROPERTY CXX_EXTENSIONS OFF)
target_link_libraries(fastgpxcli PRIVATE fastgpx-static)

# fastgpx tests

enable_testing()

add_executable(fastgpx_test fastgpx_test.cpp)
set_property(TARGET fastgpx_test PROPERTY CXX_STANDARD 23)
set_property(TARGET fastgpx_test PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET fastgpx_test PROPERTY CXX_EXTENSIONS OFF)
target_link_libraries(fastgpx_test PRIVATE Catch2::Catch2WithMain fastgpx-static)

include(CTest)
include(Catch)
catch_discover_tests(fastgpx_test)
