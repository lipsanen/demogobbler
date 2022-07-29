goblin_includes = include_directories(['../include', '.'])
goblin_lib_files = ['arena.c', 'bitstream.c', 'bitwriter.c', 'filereader.c', 'freddie_demosplicer.c', 'freddie_tostring.c', 'main.c', 'parser.c',
 'parser_netmessages.c', 'stackalloc.c', 'streams.c', 'utils.c', 'version_utils.c', 'writer.c']

if current_c_compiler == 'gcc' or current_c_compiler == 'clang'
  goblin_extra_args = ['-fno-strict-aliasing', '-fwrapv', '-pedantic-errors', '-ggdb']
else
  goblin_extra_args = []
endif

cc = meson.get_compiler('c')
goblin = static_library('demogobbler', goblin_lib_files, include_directories : goblin_includes, c_args : goblin_extra_args)
