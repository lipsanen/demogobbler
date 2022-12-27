list(APPEND DEMOGOBBLER_BENCH_SOURCES
  "arena.cpp"
  "bitstream.cpp"
  "e2e.cpp"
  "eprops.cpp"
  "hashtable.cpp"
  "main.cpp"
  "test_demos.cpp"
)

add_executable(demogobbler_bench ${DEMOGOBBLER_BENCH_SOURCES})
target_link_libraries(demogobbler_bench demogobbler benchmark::benchmark)
