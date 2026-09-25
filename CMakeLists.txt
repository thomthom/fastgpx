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

# Link-time optimization, requested by the Linux wheel build in pyproject.toml. It is enabled only
# if CMake can do it for this compiler (not, for example, for Clang without llvm-ar), so a build
# from the sdist with such a compiler warns and continues without it. A value given for
# CMAKE_INTERPROCEDURAL_OPTIMIZATION takes precedence, and fails the build if it is unsupported.
option(FASTGPX_LTO "Build with link-time optimization if the compiler supports it" OFF)

if(DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  message(STATUS "FASTGPX_LTO: ignored, CMAKE_INTERPROCEDURAL_OPTIMIZATION is set to "
                 "${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")
elseif(FASTGPX_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT FASTGPX_LTO_SUPPORTED OUTPUT FASTGPX_LTO_ERROR)
  if(FASTGPX_LTO_SUPPORTED)
    message(STATUS "FASTGPX_LTO: ON, link-time optimization enabled")
    # Before any target is defined, including the fetched dependencies, so it reaches all of them.
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else()
    message(WARNING "FASTGPX_LTO is ON, but link-time optimization is not supported with "
                    "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}; building without "
                    "it.
${FASTGPX_LTO_ERROR}")
  endif()
else()
  message(STATUS "FASTGPX_LTO: OFF")
endif()

# > The CTest module defines a BUILD_TESTING cache variable which defaults to true.
# > It is used to decide whether the module calls enable_testing() or not, so the
# > project does not have to make its own explicit call to enable_testing(). The
# > project can also use this cache variable to perform certain processing only
# > if testing is enabled. If the project has many tests that take a long time
# > to build, this can be a useful way to avoid adding them to the build when
# > they are not needed.
#
# The tests are not needed in a Python package build. scikit-build-core defines SKBUILD, so default
# BUILD_TESTING to off there, before CTest's option() would default it to on.
#
# The default is set here rather than in pyproject.toml's cmake.define table. A platform override
# in pyproject.toml replaces that table unless it sets `inherit.cmake.define = "append"`. The
# Linux override once dropped it, which turned the tests back on in the Linux wheel builds.
#
# An explicit -DBUILD_TESTING=ON still wins, and plain CMake builds keep the tests on.
if(SKBUILD AND NOT DEFINED BUILD_TESTING)
  set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree.")
endif()
include(CTest)
message(STATUS "BUILD_TESTING: ${BUILD_TESTING}")

add_subdirectory(src/cpp)
