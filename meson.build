project('demogobbler', ['c', 'cpp'], default_options : ['c_std=c11', 'cpp_std=c++17', 'default_library=both'])

current_c_compiler = meson.get_compiler('c').get_id()

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', language : 'c')
endif

if get_option('buildtype').startswith('release')
  add_project_arguments('-fno-omit-frame-pointer', language : 'c')
endif

goblin_export = include_directories(['include'])
goblin_all =  include_directories(['include', 'src'])

subdir('src')
subdir('test')
subdir('bench')
subdir('tools/demodump')
subdir('tools/listgobbler')
subdir('tools/perftest')