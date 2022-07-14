test_utils_includes = include_directories(['.'])
test_utils_extra_args = ['-pedantic-errors']
test_utils_lib_files = ['copy.cpp']
test_utils = library('test_utils', test_utils_lib_files, link_with: [goblin], include_directories : goblin_includes, cpp_args : test_utils_extra_args)
