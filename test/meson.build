gtest_proj = subproject('gtest')
gtest_dep = gtest_proj.get_variable('gtest_dep')

subdir('utils')

unittest_files = ['arena.cpp', 'baselines.cpp', 'bitstream.cpp', 'convert.cpp', 'e2e.cpp', 'ent_updates.cpp', 'hashtable.cpp', 'l4d2_version.cpp', 
'main.cpp', 'filereader.cpp', 'packet_copy.cpp', 'prop_values.cpp', 'usercmd.cpp', 'vector_array.cpp']
unittests = executable('unittests', unittest_files, link_with : [goblin, test_utils], include_directories : [goblin_all, test_utils_includes], dependencies : [gtest_dep])
test('Unit tests', unittests, workdir : meson.source_root(), timeout: 0)
