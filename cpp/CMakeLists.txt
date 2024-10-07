# cmake_minimum_required(VERSION 3.30.2)

# project(GpxReader)
# project(${SKBUILD_PROJECT_NAME} LANGUAGES CXX)

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

FetchContent_MakeAvailable(tinyxml2 pugixml)

add_executable(gpxreader main.cpp gpx.hpp gpx.cpp fastgpx.hpp fastgpx.cpp)
set_property(TARGET gpxreader PROPERTY CXX_STANDARD 23)
set_property(TARGET gpxreader PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET gpxreader PROPERTY CXX_EXTENSIONS OFF)
target_link_libraries(gpxreader PRIVATE tinyxml2 pugixml)


add_executable(fastgpx app.cpp fastgpx.hpp fastgpx.cpp)
set_property(TARGET fastgpx PROPERTY CXX_STANDARD 23)
set_property(TARGET fastgpx PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET fastgpx PROPERTY CXX_EXTENSIONS OFF)
target_link_libraries(fastgpx PRIVATE pugixml)

find_package(Python3 3.11 EXACT REQUIRED COMPONENTS Interpreter Development)

set(PYBIND11_NEWPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(gpxcpp gpxcpp.cpp gpx.hpp gpx.cpp fastgpx.hpp fastgpx.cpp)
set_property(TARGET gpxcpp PROPERTY CXX_STANDARD 23)
set_property(TARGET gpxcpp PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET gpxcpp PROPERTY CXX_EXTENSIONS OFF)
target_link_libraries(gpxcpp PRIVATE tinyxml2 pugixml)

install(TARGETS gpxcpp LIBRARY DESTINATION .)

# Assuming your CMakeLists.txt has successfully built the `gpxcpp` target
add_custom_command(TARGET gpxcpp POST_BUILD
    # Replace this with the path to your virtual environment's site-packages directory
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:gpxcpp>"  # This is the .pyd file
        "${CMAKE_SOURCE_DIR}/.venv/Lib/site-packages/gpxcpp.cp311-win_amd64.pyd"  # Adjust for your virtual env
)
