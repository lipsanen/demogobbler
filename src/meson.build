goblin_includes = include_directories(['../include', '.'])
goblin_lib_files = ['arena.c', 'bitstream.c', 'conversions.c', 'bitwriter.c', 'filereader.c', 'freddie.cpp', 'freddie_props.cpp', 'freddie_demosplicer.cpp', 
  'hashtable.c', 'parser.c', 'parser_datatables.c', 'parser_entity_state.c', 'parser_netmessages.c', 
  'parser_packetentities.c', 'parser_stringtables.c', 'parser_usercmd.c', 'streams.c', 'utils.c', 'vector_array.c', 'version_utils.c', 'writer.c']

if current_c_compiler == 'gcc' or current_c_compiler == 'clang'
  goblin_extra_args = ['-fno-strict-aliasing', '-fwrapv', '-Werror=pedantic', '-ggdb']
else
  goblin_extra_args = []
endif

cc = meson.get_compiler('c')
goblin = static_library('demogobbler', goblin_lib_files, include_directories : goblin_includes, c_args : goblin_extra_args)
