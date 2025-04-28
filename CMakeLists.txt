cmake_minimum_required(VERSION 3.30.2)

project(fastgpx)

if(NOT DEFINED FASTGPX_WARNINGS_AS_ERRORS)
  set(FASTGPX_WARNINGS_AS_ERRORS "$ENV{FASTGPX_WARNINGS_AS_ERRORS}")
endif()

option(FASTGPX_WARNINGS_AS_ERRORS "Treat C++ warnings as errors" ${FASTGPX_WARNINGS_AS_ERRORS})

message(STATUS "FASTGPX_WARNINGS_AS_ERRORS: ${FASTGPX_WARNINGS_AS_ERRORS}")

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
