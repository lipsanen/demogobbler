#include "demogobbler.h"
#include "demogobbler/conversions.h"
#include "demogobbler/datatable_types.h"
#include <stdio.h>
#include <string.h>

struct dump_state {
  const char* filepath;
  bool is_outland_01;
  bool has_died;
  int prev_health;
  int tick;
};

typedef struct dump_state dump_state;

static void print_int(dg_sendprop *prop, dg_prop_value_inner value) {
  if (prop->flag_unsigned) {
    printf("%u", value.unsigned_val);
  } else {
    printf("%d", value.signed_val);
  }
}

static const char *get_prop_name(dg_sendprop *prop) {
  static char BUFFER[1024];

  if (prop->proptype != sendproptype_datatable && prop->baseclass) {
    snprintf(BUFFER, sizeof(BUFFER), "%s.%s", prop->baseclass->name, prop->name);
    return BUFFER;
  } else {
    return prop->name;
  }
}

static void grab_header(parser_state* a, dg_header* header)
{
  dump_state* state = a->client_state;
  state->is_outland_01 = strcmp(header->map_name, "ep2_outland_01") == 0;
}

void handle_packet(parser_state *a, packet_parsed *message) {
  dump_state* state = a->client_state;
  state->tick = message->orig.preamble.tick;
}

static void handle_packetentities_parsed(parser_state *state,
                                        dg_svc_packetentities_parsed *message) {
  dump_state* dumpstate = state->client_state;

  for (size_t i = 0; i < message->data.ent_updates_count; ++i) {

    dg_ent_update *update = message->data.ent_updates + i;
    if(update->ent_index != 3)
      continue;
    dg_serverclass_data* data = state->entity_state.class_datas + update->datatable_id;

    if (update->prop_value_array_size > 0) {
      for (size_t u = 0; u < update->prop_value_array_size; ++u) {
        prop_value *value = update->prop_value_array + u;
        dg_sendprop* prop = data->props + value->prop_index;
        const char *prop_name = get_prop_name(prop);

        if(strcmp("m_iHealth.001", prop_name) == 0)
        {
          int hp = value->value.signed_val;

          if(hp < dumpstate->prev_health && !dumpstate->has_died && (!dumpstate->is_outland_01 || hp != 72))
          {
            dumpstate->has_died = true;
            int startTick = dumpstate->tick - 660;

            if(startTick > 0)
            {
              printf("demoactions\n"
              "{\n"
                "\t\"1\"\n"
                "\t{\n"
                  "\t\tfactory \"SkipAhead\"\n"
                  "\t\tname \"Unnamed1\"\n"
                  "\t\tstarttick \"0\"\n"
                  "\t\tskiptotick \"%d\"\n"
                "\t}\n"
              "}\n", startTick);
            }
            else
            {
              printf("demoactions\n"
              "{\n"
              "}\n");
            }
          }

          dumpstate->prev_health = hp;
        }   
      }
    }
  }
}

void print_help(void) {
  printf("Usage: demodump <filepath>\n");
  printf("\t--filter <string of different filters> - if filter is found within the given string then "
         "its output is included. By default all outputs are emitted\n");
  printf("\tFilters: flattened, datatables, consolecmd, customdata, header, packet"
  "stop, stringtables, synctick, usercmd, packetentities\n");
}

static bool check_if_enabled(const char* arg, const char* filter) {
  if(arg == NULL) {
    return true;
  } else {
    return strstr(arg, filter) != NULL;
  }
}

void get_settings(const char *arg, dg_settings *settings) {
  dump_state* state = (dump_state*)settings->client_state;
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    printf("Usage: healthupdates <filepath>\n");
    return 0;
  }

  dg_settings settings;
  dg_settings_init(&settings);
  settings.header_handler = grab_header;
  settings.packet_parsed_handler = handle_packet;
  settings.packetentities_parsed_handler = handle_packetentities_parsed;
  dump_state dump;
  memset(&dump, 0, sizeof(dump_state));

  dump.filepath = argv[1];
  dump.tick = 0;
  dump.prev_health = 0;
  settings.client_state = &dump;

  dg_parse_result result = dg_parse_file(&settings, dump.filepath);

  if (result.error) {
    printf("error: %s\n", result.error_message);
  }

  return dump.has_died;
}
