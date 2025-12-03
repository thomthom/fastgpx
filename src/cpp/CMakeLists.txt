include(FetchContent)

# Prevent pugixml from adding it's headers and libs to the install targets.
# Requires v1.15+.
set(PUGIXML_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG        ee86beb30e4973f5feffe3ce63bfa4fbadf72f38 # v1.15
)

FetchContent_MakeAvailable(pugixml)

if(BUILD_TESTING)

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

  FetchContent_MakeAvailable(Catch2 json)

endif()


function(set_common_properties TARGET)
  if(NOT TARGET ${TARGET})
    message(FATAL_ERROR "Target ${TARGET} does not exist!")
  endif()

  set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 23)
  set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
  set_property(TARGET ${TARGET} PROPERTY CXX_EXTENSIONS OFF)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    # Elevate warning level.
    # Ignore unknown pragmas (mac).
    target_compile_options(${TARGET} PRIVATE
        /W4
        /wd4068
    )
    if(FASTGPX_WARNINGS_AS_ERRORS)
      target_compile_options(${TARGET} PRIVATE /WX)
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      # Clang pretending to be MSVC – add Clang-specific warnings what /W4 does
      # not cover.
      target_compile_options(${TARGET} PRIVATE
          -Wshadow -Wconversion
          -Wno-unknown-pragmas
          -Wsign-conversion
      )
    endif()

    # Enable Build with Multiple Processes.
    target_compile_options(${TARGET} PRIVATE /MP)

    # Ensure MSVC report up to date version for __cplusplus macro.
    # https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-160
    target_compile_options(${TARGET} PRIVATE /Zc:__cplusplus)

  else()
    # Elevate warning level.
    # Ignore unknown pragmas (win).
    target_compile_options(${TARGET} PRIVATE
        -Wall -Wextra -pedantic
        -Wshadow -Wconversion
        -Wno-unknown-pragmas
    )
    if(FASTGPX_WARNINGS_AS_ERRORS)
      target_compile_options(${TARGET} PRIVATE -Werror)
    endif()
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
add_library(fastgpx-static STATIC ${SOURCES})
set_common_properties(fastgpx-static)
target_link_libraries(fastgpx-static PRIVATE pugixml)
target_include_directories(fastgpx-static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
# Need -fPIC for linking static library into shared library on Linux (and macOS?).
set_target_properties(fastgpx-static PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

# fastgpx python module

# https://nanobind.readthedocs.io/en/latest/building.html#preliminaries
find_package(Python 3.12
  REQUIRED COMPONENTS
    Interpreter
    Development.Module
    Development.SABIModule
)

# Detect the installed nanobind package and import it into CMake
message(STATUS "Python executable: ${Python_EXECUTABLE}")
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
find_package(nanobind CONFIG REQUIRED)

nanobind_add_module(
  # Name of the extension:
  fastgpx
  # _fastgpx_impl

  NB_STATIC

  # Marks the nanobind include directories as SYSTEM to suppress warnings.
  NB_SUPPRESS_WARNINGS

  # Target the stable ABI for Python 3.12+, which reduces
  # the number of binary wheels that must be built. This
  # does nothing on older Python versions.
  STABLE_ABI

  # Sources:
  python_fastgpx.cpp
  python_utc_chrono_nanobind.hpp
)
# Treat nanobind includes as system headers to silence warnings from nanobind
target_include_directories(fastgpx SYSTEM PRIVATE ${nanobind_INCLUDE_DIR})
set_common_properties(fastgpx)
target_link_libraries(fastgpx PRIVATE fastgpx-static)
target_compile_definitions(fastgpx PRIVATE _CRT_SECURE_NO_WARNINGS) # std::gmtime on MSVC

# https://nanobind.readthedocs.io/en/latest/typing.html
nanobind_add_stub(
  fastgpx_stub
  MODULE fastgpx

  PYTHON_PATH $<TARGET_FILE_DIR:fastgpx>
  DEPENDS fastgpx

  MARKER_FILE py.typed
  RECURSIVE

  OUTPUT_PATH ${CMAKE_SOURCE_DIR}/src
  # The OUTPUT parameter now accepts multiple values that should list each
  # generated .pyi file.
  # Note that these are not actually passed to the stub generator and purely
  # used for dependency management within CMake (e.g., to remove files when
  # executing the clean target, or to track dependencies when stub files are
  # subsequently consumed by other targets). This is necessary because CMake is
  # not able to automatically discover the generated stub paths at configuration
  # time.
  OUTPUT
    ${CMAKE_SOURCE_DIR}/src/fastgpx/__init__.pyi
    ${CMAKE_SOURCE_DIR}/src/fastgpx/geo.pyi
    ${CMAKE_SOURCE_DIR}/src/fastgpx/polyline.pyi
)

install(TARGETS fastgpx LIBRARY DESTINATION .)

# For development purposes, copy the .pyd file to the site-packages directory.
# This is not needed for the final installation, as the .pyd file will be copied
# to the site-packages directory by the Python installer.
# TODO: Use a custom VSCode command to copy the binary?
add_custom_command(TARGET fastgpx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_SOURCE_DIR}/.venv/Lib/site-packages/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:fastgpx>"  # This is the .pyd file
        "${CMAKE_SOURCE_DIR}/.venv/Lib/site-packages/"
)

if(BUILD_TESTING)

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

  include(Catch)
  catch_discover_tests(fastgpx_test)

endif()
