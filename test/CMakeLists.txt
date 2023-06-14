list(APPEND DEMOGOBBLER_TEST_SOURCES
  "arena.cpp"
  "baselines.cpp"
  "bitstream.cpp"
  "convert.cpp"
  "e2e.cpp"
  "ent_updates.cpp"
  "hashtable.cpp"
  "l4d2_version.cpp"
  "main.cpp"
  "filereader.cpp"
  "packet_copy.cpp"
  "prop_values.cpp"
  "usercmd.cpp"
  "vector_array.cpp"
  "utils/copy.cpp"
  "utils/memory_stream.cpp"
  "utils/test_demos.cpp"
)

add_executable(demogobbler_test ${DEMOGOBBLER_TEST_SOURCES})
target_link_libraries(demogobbler_test demogobbler GTest::gtest_main)
include(GoogleTest)
add_test(NAME demogobbler_test COMMAND demogobbler_test WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
