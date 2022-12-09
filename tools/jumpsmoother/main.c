#include "demogobbler.h"
#include "stdio.h"
#include <string.h>

static void handle_svcpacketentities_parsed(parser_state *_state, dg_svc_packetentities_parsed *message) {
  for(size_t i=0; i < message->data.ent_updates_count; ++i) {
    dg_ent_update* update = message->data.ent_updates + i;

    if(update->ent_index == 0)
      continue;
    else if(update->ent_index > 1)
      break;

    for(size_t prop_index; prop_index < update->prop_value_array_size; ++prop_index) {
      prop_value value = update->prop_value_array[prop_index];
      if(strstr(value.prop->name, "m_vecViewOffset") != NULL)
      {
        //
      }
    }
  }
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    printf("Usage: jumpsmoother <filepath>\n");
    return 0;
  }

  dg_settings settings;
  dg_settings_init(&settings);
  settings.packetentities_parsed_handler = handle_svcpacketentities_parsed;

  dg_parse_file(&settings, argv[1]);

  return 0;
}