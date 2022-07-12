gtest_proj = subproject('gtest')
gtest_dep = gtest_proj.get_variable('gtest_dep')

unittest_files = ['main.cpp', 'basic.cpp', 'filereader.cpp']
unittests = executable('unittests', unittest_files, link_with : [goblin], include_directories : goblin_all, dependencies : [gtest_dep])
test('Unit tests', unittests)
