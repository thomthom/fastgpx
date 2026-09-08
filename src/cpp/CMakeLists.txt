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
# No source in this target currently calls a CRT function that MSVC flags as deprecated (the
# datetime caster uses gmtime_s/localtime_s). Kept as a safety net for standard C calls.
target_compile_definitions(fastgpx PRIVATE _CRT_SECURE_NO_WARNINGS)

# https://nanobind.readthedocs.io/en/latest/typing.html
#
# Equivalent to nanobind_add_stub(fastgpx_stub MODULE fastgpx RECURSIVE ...),
# except that stubgen runs with `python -S`. Once the package has been installed
# in editable mode (`uv sync`, `pip install -e .`), scikit-build-core adds an
# import hook to site-packages that redirects `import fastgpx` to the module
# *installed* in site-packages. Without -S that hook wins over the build
# directory, so the stubs would be generated from the previous build and, on
# Linux/macOS, be written to `src/fastgpx.abi3/` instead of `src/fastgpx/`.
# -S skips site-packages so the freshly built module is imported instead.
# stubgen.py only depends on the standard library.
# NB_DIR is set by find_package(nanobind); same lookup as nanobind_add_stub().
if(EXISTS "${NB_DIR}/src/stubgen.py")
  set(FASTGPX_STUBGEN "${NB_DIR}/src/stubgen.py")
elseif(EXISTS "${NB_DIR}/stubgen.py")
  set(FASTGPX_STUBGEN "${NB_DIR}/stubgen.py")
else()
  message(FATAL_ERROR "Could not locate nanobind's stubgen.py in ${NB_DIR}")
endif()
set(FASTGPX_STUB_OUTPUTS
  ${CMAKE_SOURCE_DIR}/src/fastgpx/__init__.pyi
  ${CMAKE_SOURCE_DIR}/src/fastgpx/geo.pyi
  ${CMAKE_SOURCE_DIR}/src/fastgpx/polyline.pyi
  ${CMAKE_SOURCE_DIR}/src/fastgpx/py.typed
)
add_custom_command(
  OUTPUT ${FASTGPX_STUB_OUTPUTS}
  COMMAND "${Python_EXECUTABLE}" -S "${FASTGPX_STUBGEN}"
    -q
    -r                      # RECURSIVE: also generate fastgpx.geo and fastgpx.polyline stubs
    -i "$<TARGET_FILE_DIR:fastgpx>"
    -M "${CMAKE_SOURCE_DIR}/src/fastgpx/py.typed"
    -m fastgpx
    -O "${CMAKE_SOURCE_DIR}/src"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS fastgpx "${FASTGPX_STUBGEN}"
  COMMENT "Generating fastgpx type stubs"
)
add_custom_target(fastgpx_stub ALL DEPENDS ${FASTGPX_STUB_OUTPUTS})

install(TARGETS fastgpx LIBRARY DESTINATION .)

# For development purposes, copy the extension module to the site-packages
# directory of the Python interpreter found above (normally the project .venv).
# This is not needed for the final installation, as the module will be copied
# to the site-packages directory by the Python installer.
# Python_SITEARCH resolves to `.venv\Lib\site-packages` on Windows and
# `.venv/lib/pythonX.Y/site-packages` on Linux/macOS.
# Skipped when scikit-build-core drives the build (SKBUILD is set), since the
# wheel is then installed into site-packages by uv/pip anyway.
# TODO: Use a custom VSCode command to copy the binary?
if(NOT SKBUILD)
  add_custom_command(TARGET fastgpx POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
          "${Python_SITEARCH}/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_FILE:fastgpx>"  # This is the .pyd/.so file
          "${Python_SITEARCH}/"
  )
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
    fastgpx/test_data_test.cpp
  )
  add_executable(fastgpx_test ${TEST_UTILS} ${TEST_SOURCES})
  set_common_properties(fastgpx_test)
  target_link_libraries(fastgpx_test PRIVATE Catch2::Catch2WithMain fastgpx-static nlohmann_json)
  target_compile_definitions(fastgpx_test PRIVATE FASTGPX_PROJECT_DIR="${CMAKE_SOURCE_DIR}")

  include(Catch)
  catch_discover_tests(fastgpx_test)

endif()
