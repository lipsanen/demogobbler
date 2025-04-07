add_executable(democonverter democonverter/main.cpp)
target_link_libraries(democonverter demogobbler)

add_executable(demodump demodump/main.c)
target_link_libraries(demodump demogobbler)

add_executable(demosplicer demosplicer/main.cpp)
target_link_libraries(demosplicer demogobbler)

add_executable(listgobbler listgobbler/main.c)
target_link_libraries(listgobbler demogobbler)

add_executable(perftest perftest/main.c)
target_link_libraries(perftest demogobbler)

add_executable(propdiffer propdiffer/main.cpp)
target_link_libraries(propdiffer demogobbler)

add_executable(healthupdates healthupdates/main.c)
target_link_libraries(healthupdates demogobbler)

add_executable(demosuperimposer demosuperimposer/main.cpp)
target_link_libraries(demosuperimposer demogobbler)

add_executable(speedpolice speedpolice.c)
target_link_libraries(speedpolice demogobbler)
