# Fuzz targets for the fastgpx parsers.
#
# Each fuzz_<name>.cpp defines `LLVMFuzzerTestOneInput`. With Clang (FASTGPX_LIBFUZZER) the
# targets link against libFuzzer and can be run as fuzzers:
#
#   fuzz_gpx -dict=src/cpp/fuzz/gpx.dict <corpus-dir> src/cpp/fuzz/corpus/gpx
#
# With other compilers the same sources are linked with standalone_main.cpp, which replays the
# files (or directories of files) given on the command line through the fuzz entry point. That
# gives every compiler a regression run over the seed corpora, which is also registered with
# CTest as fuzz_<name>_corpus.
#
# The sanitizer and coverage instrumentation is configured in the top-level CMakeLists.txt.

function(fastgpx_add_fuzzer NAME)
  set(TARGET fuzz_${NAME})
  add_executable(${TARGET} fuzz_${NAME}.cpp)
  set_common_properties(${TARGET})
  target_link_libraries(${TARGET} PRIVATE fastgpx-static)

  set(CORPUS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/corpus/${NAME})

  if(FASTGPX_LIBFUZZER)
    target_link_options(${TARGET} PRIVATE -fsanitize=fuzzer)
    # -runs=0 executes every input in the corpus once without mutating anything.
    add_test(NAME ${TARGET}_corpus COMMAND ${TARGET} -runs=0 ${CORPUS_DIR} ${ARGN})
  else()
    target_sources(${TARGET} PRIVATE standalone_main.cpp)
    add_test(NAME ${TARGET}_corpus COMMAND ${TARGET} ${CORPUS_DIR} ${ARGN})
  endif()
endfunction()

# Extra arguments are additional seed directories for the corpus regression test.
fastgpx_add_fuzzer(gpx
  ${CMAKE_SOURCE_DIR}/gpx/test
  ${CMAKE_SOURCE_DIR}/gpx/third-party/gpxpy
)
fastgpx_add_fuzzer(polyline)
fastgpx_add_fuzzer(polyline_encode)
fastgpx_add_fuzzer(datetime)
