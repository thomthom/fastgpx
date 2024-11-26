include(FetchContent)

set(TINYXML2_BUILD_TESTING OFF CACHE BOOL "Disable tinyxml2 tests" FORCE)

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

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(tinyxml2 pugixml Catch2 json)

function(set_common_properties TARGET)
  if(NOT TARGET ${TARGET})
    message(FATAL_ERROR "Target ${TARGET} does not exist!")
  endif()

  set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 23)
  set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
  set_property(TARGET ${TARGET} PROPERTY CXX_EXTENSIONS OFF)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Elevate warning level.
    # Ignore unknown pragmas (mac).
    target_compile_options(${TARGET} PRIVATE
        /W4 /WX
        /wd4068
    )

    # Enable Build with Multiple Processes.
    target_compile_options(${TARGET} PRIVATE /MP)

    # Ensure MSVC report up to date version for __cplusplus macro.
    # https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-160
    target_compile_options(${TARGET} PRIVATE /Zc:__cplusplus)
  else()
    # Elevate warning level.
    # Ignore unknown pragmas (win).
    target_compile_options(${TARGET} PRIVATE
        -Wall -Wextra -pedantic -Werror
        -Wshadow -Wconversion
        -Wno-unknown-pragmas
    )
  endif()
endfunction()

# fastgpx static library

set(SOURCES
  fastgpx/datetime.hpp
  fastgpx/datetime.cpp
  fastgpx/errors.hpp
  fastgpx/errors.cpp
  fastgpx/fastgpx.hpp
  fastgpx/fastgpx.cpp
  fastgpx/filesystem.hpp
  fastgpx/filesystem.cpp
  fastgpx/geom.hpp
  fastgpx/geom.cpp
  fastgpx/polyline.hpp
  fastgpx/polyline.cpp
)
add_library(fastgpx-static ${SOURCES})
set_common_properties(fastgpx-static)
target_link_libraries(fastgpx-static PUBLIC tinyxml2 pugixml)
target_include_directories(fastgpx-static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# fastgpx python module

find_package(Python3 3.11 EXACT REQUIRED COMPONENTS Interpreter Development)

set(PYBIND11_NEWPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(fastgpx python_fastgpx.cpp python_utc_chrono.hpp)
set_common_properties(fastgpx)
target_link_libraries(fastgpx PRIVATE fastgpx-static)
target_compile_definitions(fastgpx PRIVATE _CRT_SECURE_NO_WARNINGS) # std::gmtime on MSVC

install(TARGETS fastgpx LIBRARY DESTINATION .)

add_custom_command(TARGET fastgpx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:fastgpx>"  # This is the .pyd file
        "${CMAKE_SOURCE_DIR}/.venv/Lib/site-packages/fastgpx.cp311-win_amd64.pyd"
)

# fastgpx debug app

add_executable(fastgpxcli app.cpp)
set_common_properties(fastgpxcli)
target_link_libraries(fastgpxcli PRIVATE fastgpx-static)

# fastgpx tests

set(TEST_UTILS
  fastgpx/test_data.hpp
  fastgpx/test_data.cpp
)
set(TEST_SOURCES
  fastgpx/datetime_test.cpp
  fastgpx/errors_test.cpp
  fastgpx/fastgpx_test.cpp
  fastgpx/filesystem_test.cpp
  fastgpx/geom_test.cpp
  fastgpx/test_data_test.cpp
)
add_executable(fastgpx_test ${TEST_UTILS} ${TEST_SOURCES})
set_common_properties(fastgpx_test)
target_link_libraries(fastgpx_test PRIVATE Catch2::Catch2WithMain fastgpx-static nlohmann_json)
target_compile_definitions(fastgpx_test PRIVATE FASTGPX_PROJECT_DIR="${CMAKE_SOURCE_DIR}")

include(CTest)
include(Catch)
catch_discover_tests(fastgpx_test)

# experimental Python C Extension

pybind11_add_module(gpxcpp gpxcpp.cpp gpx.hpp gpx.cpp)
set_common_properties(gpxcpp)
target_link_libraries(gpxcpp PRIVATE tinyxml2 pugixml fastgpx-static)

install(TARGETS gpxcpp LIBRARY DESTINATION .)

add_custom_command(TARGET gpxcpp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:gpxcpp>"  # This is the .pyd file
        "${CMAKE_SOURCE_DIR}/.venv/Lib/site-packages/gpxcpp.cp311-win_amd64.pyd"
)

# experimantal CLI app

add_executable(gpxreader main.cpp gpx.hpp gpx.cpp)
set_common_properties(gpxreader)
target_link_libraries(gpxreader PRIVATE tinyxml2 pugixml fastgpx-static)
