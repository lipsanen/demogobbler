goblin_includes = include_directories(['../include', '.'])
goblin_lib_files = ['bitstream.c', 'filereader.c', 'freddie.c', 'main.c', 'parser.c',
 'parser_netmessages.c', 'stackalloc.c', 'streams.c', 'utils.c', 'version_utils.c', 'writer.c']
if current_c_compiler == 'clang' or current_c_compiler == 'gcc'
  goblin_extra_args = ['-fno-strict-aliasing', '-pedantic-errors']
else
  goblin_extra_args = []
endif

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
goblin = static_library('demogobbler', goblin_lib_files, include_directories : goblin_includes, c_args : goblin_extra_args, dependencies: m_dep)
