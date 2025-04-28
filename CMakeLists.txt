cmake_minimum_required(VERSION 3.30.2)

project(fastgpx)

# A build flag that allows to switch build logic for production build.
# For instance, this allow `pip install` to build the package without having
# warnings treated as errors. Whereas the default build, local and CI, will treat
# warnings as errors.
option(FASTGPX_PRODUCTION_BUILD "Meant to be used for the pip install build path." OFF)

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
