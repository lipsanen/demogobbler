test_utils_includes = include_directories(['.'])

if current_c_compiler == 'clang' or current_c_compiler == 'gcc'
  test_utils_extra_args = ['-pedantic-errors']
else
  test_utils_extra_args = []
endif

test_utils_lib_files = ['copy.cpp', 'memory_stream.cpp']
test_utils = static_library('test_utils', test_utils_lib_files, link_with: [goblin], include_directories : goblin_includes, dependencies : [gtest_dep], cpp_args : test_utils_extra_args)
