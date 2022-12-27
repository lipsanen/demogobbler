
list(APPEND DEMOGOBBLER_SOURCES
  "arena.c"
  "bitstream.c"
  "conversions.c"
  "bitwriter.c"
  "filereader.c"
  "freddie.cpp"
  "freddie_props.cpp"
  "freddie_demosplicer.cpp"
  "hashtable.c"
  "parser.c"
  "parser_datatables.c"
  "parser_entity_state.c"
  "parser_netmessages.c"
  "parser_packetentities.c"
  "parser_stringtables.c"
  "parser_usercmd.c"
  "streams.c"
  "utils.c"
  "vector_array.c"
  "version_utils.c"
  "writer.c"
)

add_library(demogobbler ${DEMOGOBBLER_SOURCES})
target_include_directories(demogobbler PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_include_directories(demogobbler INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
