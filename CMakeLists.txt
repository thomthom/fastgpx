cmake_minimum_required(VERSION 3.30.2)

project(fastgpx)

if(NOT DEFINED FASTGPX_WARNINGS_AS_ERRORS)
  set(FASTGPX_WARNINGS_AS_ERRORS "$ENV{FASTGPX_WARNINGS_AS_ERRORS}")
endif()

option(FASTGPX_WARNINGS_AS_ERRORS "Treat C++ warnings as errors" ${FASTGPX_WARNINGS_AS_ERRORS})

message(STATUS "FASTGPX_WARNINGS_AS_ERRORS: ${FASTGPX_WARNINGS_AS_ERRORS}")

# The Python extension module needs a Python interpreter with nanobind installed. Turn it off for
# pure C++ builds, e.g. fuzzing, where neither is needed.
option(FASTGPX_BUILD_PYTHON_MODULE "Build the fastgpx Python extension module" ON)

# Fuzz targets live in src/cpp/fuzz. With Clang they are libFuzzer binaries instrumented with
# AddressSanitizer and UndefinedBehaviorSanitizer. With other compilers they are built with a
# standalone driver that replays the seed corpora (see the fuzz CMakeLists.txt for details).
option(FASTGPX_BUILD_FUZZERS "Build the fuzz targets" OFF)

if(FASTGPX_BUILD_FUZZERS)
  message(STATUS "FASTGPX_BUILD_FUZZERS: ON")
  # The instrumentation is applied at directory scope, before any target is defined, so that the
  # whole parsing path is covered - including pugixml, which is fetched and built from source.
  # Use a dedicated build directory for fuzzing.
  #
  # Keep `assert` enabled regardless of the build type. Assertions are a cheap oracle for the
  # fuzzer, so a fuzzing build should always have them on.
  foreach(config RELEASE RELWITHDEBINFO MINSIZEREL)
    foreach(lang C CXX)
      string(REGEX REPLACE "[/-]DNDEBUG" "" CMAKE_${lang}_FLAGS_${config} "${CMAKE_${lang}_FLAGS_${config}}")
    endforeach()
  endforeach()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    set(FASTGPX_LIBFUZZER ON)
    add_compile_options(
      -fsanitize=fuzzer-no-link,address,undefined
      -fno-sanitize-recover=all
      -fno-omit-frame-pointer
      -g
    )
    add_link_options(-fsanitize=address,undefined)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|AppleClang)$")
    # No libFuzzer runtime, but the sanitizers are available.
    set(FASTGPX_LIBFUZZER OFF)
    add_compile_options(
      -fsanitize=address,undefined
      -fno-sanitize-recover=all
      -fno-omit-frame-pointer
      -g
    )
    add_link_options(-fsanitize=address,undefined)
  else()
    set(FASTGPX_LIBFUZZER OFF)
    message(STATUS "No libFuzzer/sanitizer support configured for ${CMAKE_CXX_COMPILER_ID}; "
                   "the fuzz targets are built with the standalone corpus runner only.")
  endif()
  if(FASTGPX_BUILD_PYTHON_MODULE)
    message(WARNING "FASTGPX_BUILD_FUZZERS is ON while FASTGPX_BUILD_PYTHON_MODULE is also ON. "
                    "The Python module would be built with the fuzzing instrumentation. "
                    "Consider -DFASTGPX_BUILD_PYTHON_MODULE=OFF for fuzzing builds.")
  endif()
endif()

# > The CTest module defines a BUILD_TESTING cache variable which defaults to true.
# > It is used to decide whether the module calls enable_testing() or not, so the
# > project does not have to make its own explicit call to enable_testing(). The
# > project can also use this cache variable to perform certain processing only
# > if testing is enabled. If the project has many tests that take a long time
# > to build, this can be a useful way to avoid adding them to the build when
# > they are not needed.
#
# The BUILD_TESTING is set to false in pyproject.toml, so the tests will not build
# when building the Python package.
include(CTest)

add_subdirectory(src/cpp)
