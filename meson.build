project('demogobbler', ['c', 'cpp'], default_options : ['c_std=c11', 'cpp_std=c++17', 'default_library=both'])

current_c_compiler = meson.get_compiler('c', native : false).get_id()
if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', language : ['c', 'cpp'])
endif

if (current_c_compiler == 'clang' or current_c_compiler == 'gcc') and host_machine.cpu() != 'wasm32'
  add_project_arguments('-fno-omit-frame-pointer', language : ['c', 'cpp'])
  add_project_arguments('-ggdb', language : ['c', 'cpp'])
  add_project_arguments('-Werror-implicit-function-declaration', language : ['c'])
  add_project_arguments('-Werror=incompatible-pointer-types', language : ['c'])
endif

goblin_export = include_directories(['include'])
goblin_all =  include_directories(['include', 'src'])

subdir('src')

# No tests/bench for wasm32, gtest and gbenchmark dont compile
if host_machine.cpu() != 'wasm32'
subdir('test')
subdir('bench')
endif

subdir('tools/democonverter')
subdir('tools/demodump')
subdir('tools/demosplicer')
subdir('tools/listgobbler')
subdir('tools/perftest')
subdir('tools/propdiffer')
