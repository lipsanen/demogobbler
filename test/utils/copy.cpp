#include "demogobbler.h"


writer w;

#define DECLARE_WRITE_FUNC(type) static void type ##_handler(demogobbler_ ## type *message) {\
  demogobbler_write_ ## type (&w, message); \
}

DECLARE_WRITE_FUNC(consolecmd);
DECLARE_WRITE_FUNC(customdata);
DECLARE_WRITE_FUNC(datatables);
DECLARE_WRITE_FUNC(header);
DECLARE_WRITE_FUNC(packet);
DECLARE_WRITE_FUNC(stringtables);
DECLARE_WRITE_FUNC(stop);
DECLARE_WRITE_FUNC(synctick);
DECLARE_WRITE_FUNC(usercmd);

void copy_demo(const char* filepath, const char* output)
{
  demogobbler_writer_init(&w);
  demogobbler_writer_open_file(&w, output);
  if(!w.error_set)
  {
    demogobbler_parser parser;
    demogobbler_settings settings;
    demogobbler_settings_init(&settings);

    settings.consolecmd_handler = consolecmd_handler;
    settings.customdata_handler = customdata_handler;
    settings.datatables_handler = datatables_handler;
    settings.header_handler = header_handler;
    settings.packet_handler = packet_handler;
    settings.stop_handler = stop_handler;
    settings.stringtables_handler = stringtables_handler;
    settings.synctick_handler = synctick_handler;
    settings.usercmd_handler = usercmd_handler;

    demogobbler_parser_init(&parser, &settings);
    demogobbler_parser_parse_file(&parser, filepath);
    demogobbler_parser_free(&parser);

    demogobbler_writer_close(&w);
  }
  demogobbler_writer_free(&w);
}