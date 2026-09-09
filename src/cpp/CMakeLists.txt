include(FetchContent)

# Prevent pugixml from adding its headers and libs to the install targets.
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

  set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )

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

add_library(fastgpx-static STATIC)
set_common_properties(fastgpx-static)
target_link_libraries(fastgpx-static PRIVATE pugixml)
set_target_properties(fastgpx-static PROPERTIES
  # Need -fPIC for linking static library into shared library on Linux (and macOS?).
  POSITION_INDEPENDENT_CODE ON
)
target_sources(fastgpx-static
  PUBLIC
    FILE_SET fastgpx_static_headers
    TYPE HEADERS
    FILES
      fastgpx/datetime.hpp
      fastgpx/errors.hpp
      fastgpx/fastgpx.hpp
      fastgpx/filesystem.hpp
      fastgpx/geom.hpp
      fastgpx/polyline.hpp
    PRIVATE
      fastgpx/datetime.cpp
      fastgpx/errors.cpp
      fastgpx/fastgpx.cpp
      fastgpx/filesystem.cpp
      fastgpx/geom.cpp
      fastgpx/polyline.cpp
)

# fastgpx python module

if(FASTGPX_BUILD_PYTHON_MODULE)

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

# The extension is built in nanobind's split mode: libnanobind is a separate backend module that
# the extension imports at load time. Wheels use the `nanobind-backend` package from PyPI. That
# backend is built with the release CRT on Windows, and a Debug build of the extension links the
# debug CRT, which the backend rejects with "platform ABI ... _debug". This option builds a
# backend from nanobind's sources in this tree instead, in the same configuration as the
# extension, so that Debug builds load. `.vscode/settings.json` turns it on for IDE builds, and
# single-config Debug builds default to it. Wheel builds leave it off.
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_CONFIGURATION_TYPES)
  set(FASTGPX_LOCAL_NANOBIND_BACKEND_DEFAULT ON)
else()
  set(FASTGPX_LOCAL_NANOBIND_BACKEND_DEFAULT OFF)
endif()
option(FASTGPX_LOCAL_NANOBIND_BACKEND
  "Build the nanobind backend module in this tree instead of using the nanobind-backend package"
  ${FASTGPX_LOCAL_NANOBIND_BACKEND_DEFAULT})
message(STATUS "FASTGPX_LOCAL_NANOBIND_BACKEND: ${FASTGPX_LOCAL_NANOBIND_BACKEND}")

if(FASTGPX_LOCAL_NANOBIND_BACKEND)
  nanobind_add_backend(fastgpx_nanobind_backend NB_SUPPRESS_WARNINGS)
  set(FASTGPX_NANOBIND_BACKEND fastgpx_nanobind_backend)
else()
  set(FASTGPX_NANOBIND_BACKEND nanobind_backend)
endif()

nanobind_add_module(
  # Name of the extension:
  fastgpx

  # nanobind split mode: libnanobind is not linked into the extension but provided at runtime by
  # a backend module, which ships the Python-version-specific fast paths. The extension itself is
  # a stable ABI module. Measured 17-25% faster than the linked stable ABI build on the list
  # conversion and polyline paths; see benchmarks/nanobind3.md.
  BACKEND_MODULE ${FASTGPX_NANOBIND_BACKEND}

  # Split mode always targets the stable ABI, with a 3.10 floor by default. Pin the floor so that
  # plain CMake builds match the wheels (`wheel.py-api = "cp312"` in pyproject.toml).
  STABLE_ABI_VERSION 3.12

  # Marks the nanobind include directories as SYSTEM to suppress warnings.
  NB_SUPPRESS_WARNINGS

  # Sources:
  python_fastgpx.cpp
  python_utc_chrono_nanobind.hpp
)
set_common_properties(fastgpx)
target_link_libraries(fastgpx PRIVATE fastgpx-static)
if(FASTGPX_LOCAL_NANOBIND_BACKEND)
  # The backend is imported by the extension, so it must exist before the stub generator runs and
  # travel with the extension wherever it is installed.
  add_dependencies(fastgpx fastgpx_nanobind_backend)
  install(TARGETS fastgpx_nanobind_backend LIBRARY DESTINATION .)
endif()
# No source in this target currently calls a CRT function that MSVC flags as deprecated (the
# datetime caster uses the C++20 calendar types, not the C time functions). Kept as a safety net
# for standard C calls.
target_compile_definitions(fastgpx PRIVATE _CRT_SECURE_NO_WARNINGS)

# https://nanobind.readthedocs.io/en/latest/typing.html
# The stub generator imports the freshly built module, so the Python running it needs the
# `nanobind-backend` package (a project dependency, installed by `uv sync` and by the build
# backend's requirements).
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
if(FASTGPX_LOCAL_NANOBIND_BACKEND)
  add_custom_command(TARGET fastgpx_nanobind_backend POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
          "${CMAKE_SOURCE_DIR}/.venv/Lib/site-packages/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_FILE:fastgpx_nanobind_backend>"
          "${CMAKE_SOURCE_DIR}/.venv/Lib/site-packages/"
  )
endif()

endif() # FASTGPX_BUILD_PYTHON_MODULE

# fastgpx fuzz targets

if(FASTGPX_BUILD_FUZZERS)
  add_subdirectory(fuzz)
endif()

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
    fastgpx/polyline_test.cpp
    fastgpx/test_data_test.cpp
  )
  add_executable(fastgpx_test ${TEST_UTILS} ${TEST_SOURCES})
  set_common_properties(fastgpx_test)
  target_link_libraries(fastgpx_test PRIVATE Catch2::Catch2WithMain fastgpx-static nlohmann_json)
  target_compile_definitions(fastgpx_test PRIVATE FASTGPX_PROJECT_DIR="${CMAKE_SOURCE_DIR}")

  include(Catch)
  catch_discover_tests(fastgpx_test)

endif()
