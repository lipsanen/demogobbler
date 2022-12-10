#include "demogobbler.h"
#include "demogobbler/conversions.h"
#include "demogobbler/datatable_types.h"
#define XXH_INLINE_ALL
#include "xxhash.h"
#include <stdio.h>
#include <string.h>

void print_data(const void* data, size_t bytes) {
  uint8_t* ptr = (uint8_t*)data;
  printf("uint8_t msg[] = { ");
  for(size_t i=0; i < bytes; ++i) {
    if(i == 0) {
      printf("0x%x", ptr[i]);
    } else {
      printf(", 0x%x", ptr[i]);
    }
  }
  printf(" };\n");
}

void print_header(parser_state *a, dg_header *header) {
  printf("ID: %s\n", header->ID);
  printf("Demo protocol: %d\n", header->demo_protocol);
  printf("Net protocol: %d\n", header->net_protocol);
  printf("Server name: %s\n", header->server_name);
  printf("Client name: %s\n", header->client_name);
  printf("Map name: %s\n", header->map_name);
  printf("Game directory: %s\n", header->game_directory);
  printf("Seconds: %f\n", header->seconds);
  printf("Tick count: %d\n", header->tick_count);
  printf("Frame count: %d\n", header->frame_count);
  printf("Signon length: %d\n", header->signon_length);
}

struct dump_state {
  dg_demver_data version_data;
  bool print_raw_data;
};

typedef struct dump_state dump_state;

#define PRINT_MESSAGE_PREAMBLE(name)                                                               \
  printf(#name ": Tick %d, Slot %d\n", message->preamble.tick, message->preamble.slot);

void print_consolecmd(parser_state *a, dg_consolecmd *message) {
  PRINT_MESSAGE_PREAMBLE(consolecmd);
  printf("\t%s\n", message->data);
}

void print_customdata(parser_state *a, dg_customdata *message) {
  PRINT_MESSAGE_PREAMBLE(customdata);
}

void print_packet_orig(parser_state *a, dg_packet *message) {
  dump_state *state = a->client_state;
  if (message->preamble.type == dg_type_signon) {
    printf("Signon: Tick %d, Slot %d, SizeBytes %d\n", message->preamble.tick,
           message->preamble.slot, message->size_bytes);
  } else {
    printf("Packet: Tick %d, Slot %d, SizeBytes %d\n", message->preamble.tick,
           message->preamble.slot, message->size_bytes);
  }

  for (int i = 0; i < state->version_data.cmdinfo_size; ++i) {
    printf("Cmdinfo %d:\n", i);
    printf("\tInterp flags %d\n", message->cmdinfo[i].interp_flags);
    printf("\tLocal viewangles (%f, %f, %f)\n", message->cmdinfo[i].local_viewangles.x,
           message->cmdinfo[i].local_viewangles.y, message->cmdinfo[i].local_viewangles.z);
    printf("\tLocal viewangles 2 (%f, %f, %f)\n", message->cmdinfo[i].local_viewangles2.x,
           message->cmdinfo[i].local_viewangles2.y, message->cmdinfo[i].local_viewangles2.z);
    printf("\tView angles (%f, %f, %f)\n", message->cmdinfo[i].view_angles.x,
           message->cmdinfo[i].view_angles.y, message->cmdinfo[i].view_angles.z);
    printf("\tView angles2 (%f, %f, %f)\n", message->cmdinfo[i].view_angles2.x,
           message->cmdinfo[i].view_angles2.y, message->cmdinfo[i].view_angles2.z);
    printf("\tOrigin (%f, %f, %f)\n", message->cmdinfo[i].view_origin.x,
           message->cmdinfo[i].view_origin.y, message->cmdinfo[i].view_origin.z);
    printf("\tOrigin 2 (%f, %f, %f)\n", message->cmdinfo[i].view_origin2.x,
           message->cmdinfo[i].view_origin2.y, message->cmdinfo[i].view_origin2.z);
  }

  printf("In sequence: %d, Out sequence: %d\n", message->in_sequence, message->out_sequence);
  if(state->print_raw_data) {
    print_data(message->data, message->size_bytes);
  }
  printf("Messages:\n");
}

void print_packet(parser_state *a, packet_parsed *message) {
  print_packet_orig(a, &message->orig);

  for (size_t i = 0; i < message->message_count; ++i) {
    packet_net_message *netmsg = message->messages + i;
    if (netmsg->mtype == svc_create_stringtable) {
      struct dg_svc_create_stringtable msg = netmsg->message_svc_create_stringtable;
      printf("\tsvc_create_stringtable %s, %u entries\n", msg.name, msg.num_entries);
    } else if (netmsg->mtype == svc_serverinfo) {
      struct dg_svc_serverinfo *msg = netmsg->message_svc_serverinfo;
      printf("\tsvc_serverinfo\n");
      printf("\t\tClient CRC: %u\n", msg->client_crc);
      printf("\t\tMap CRC: %u\n", msg->map_crc);
      printf("\t\tStringtable CRC: %u\n", msg->stringtable_crc);
    } else if (netmsg->mtype == svc_packet_entities) {
      printf("\tsvc_packetentities\n");
    } else if (netmsg->mtype == svc_sounds) {
      printf("\tsvc_sounds\n");
    } else if (netmsg->mtype == svc_update_stringtable) {
      printf("\tsvc_update_stringtable\n");
    } else if (netmsg->mtype == svc_bsp_decal) {
      printf("\tsvc_bsp_decal\n");
    } else if (netmsg->mtype == svc_classinfo) {
      printf("\tsvc_classinfo\n");
    } else if (netmsg->mtype == svc_cmd_key_values) {
      printf("\tsvc_cmd_key_values\n");
    } else if (netmsg->mtype == svc_crosshair_angle) {
      printf("\tsvc_crosshair_angle\n");
    } else if (netmsg->mtype == svc_entity_message) {
      printf("\tsvc_entity_message\n");
    } else if (netmsg->mtype == svc_fixangle) {
      printf("\tsvc_fixangle\n");
    } else if (netmsg->mtype == svc_game_event) {
      printf("\tsvc_game_event\n");
    } else if (netmsg->mtype == svc_game_event_list) {
      printf("\tsvc_game_event_list\n");
    } else if (netmsg->mtype == svc_get_cvar_value) {
      printf("\tsvc_get_cvar_value\n");
    } else if (netmsg->mtype == svc_menu) {
      printf("\tsvc_menu\n");
    } else if (netmsg->mtype == svc_paintmap_data) {
      printf("\tsvc_paintmap_data\n");
    } else if (netmsg->mtype == svc_prefetch) {
      printf("\tsvc_prefetch\n");
    } else if (netmsg->mtype == svc_setpause) {
      printf("\tsvc_setpause\n");
    } else if (netmsg->mtype == svc_setview) {
      printf("\tsvc_setview\n");
    } else if (netmsg->mtype == svc_splitscreen) {
      printf("\tsvc_splitscreen\n");
    } else if (netmsg->mtype == svc_temp_entities) {
      printf("\tsvc_temp_entities\n");
    } else if (netmsg->mtype == svc_user_message) {
      printf("\tsvc_user_message\n");
    } else if (netmsg->mtype == svc_voice_data) {
      printf("\tsvc_voice_data\n");
    } else if (netmsg->mtype == svc_voice_init) {
      printf("\tsvc_voice_init\n");
    } else if (netmsg->mtype == net_disconnect) {
      printf("\tnet_disconnect\n");
    } else if (netmsg->mtype == net_file) {
      printf("\tnet_file\n");
    } else if (netmsg->mtype == net_nop) {
      printf("\tnet_nop\n");
    } else if (netmsg->mtype == net_setconvar) {
      printf("\tnet_setconvar\n");
    } else if (netmsg->mtype == net_stringcmd) {
      printf("\tnet_stringcmd\n");
    } else if (netmsg->mtype == net_tick) {
      const float scale = 100000.0f;
      struct dg_net_tick msg = netmsg->message_net_tick;
      printf("\tnet_tick time %f dev %f tick %d\n", msg.host_frame_time / scale,
             msg.host_frame_time_std_dev / scale, msg.tick);
    } else if (netmsg->mtype == net_signonstate) {
      printf("\tnet_signonstate\n");
    } else if (netmsg->mtype == net_splitscreen_user) {
      printf("\tnet_splitscreen_user\n");
    } else {
      printf("\tunformatted msg %u\n", netmsg->mtype);
    }
  }
}

void print_stringtables_parsed(parser_state *a, dg_stringtables_parsed *message) {
  dump_state *state = a->client_state;

  uint64_t hash = XXH64(message->orig.data, message->orig.size_bytes, 0);
  printf("Stringtables: Tick %d, Slot %d, Hash 0x%lx, SizeBytes %d\n", message->orig.preamble.tick,
         message->orig.preamble.slot, hash, message->orig.size_bytes);

  if(state->print_raw_data) {
    print_data(message->orig.data, message->orig.size_bytes);
  }

  for (size_t i = 0; i < message->tables_count; ++i) {
    dg_stringtable *table = message->tables + i;
    printf("\t[%lu] %s entries:\n", i, table->table_name);
    for (size_t u = 0; u < table->entries_count; ++u) {
      dg_stringtable_entry *entry = table->entries + u;
      if (entry->size == 1) {
        uint32_t value = bitstream_read_uint(&entry->data, 8);
        printf("\t\t[%lu] %s - 0x%x\n", u, entry->name, value);
      } else {
        printf("\t\t[%lu] %s - %u bytes\n", u, entry->name, entry->size);
      }
    }

    if (table->has_classes) {
      printf("\t[%lu] %s classes:\n", i, table->table_name);
      for (size_t u = 0; u < table->classes_count; ++u) {
        dg_stringtable_entry *entry = table->classes + u;
        printf("\t\t[%lu] %s - %u bytes\n", u, entry->name, entry->size);
      }
    }
  }
}

void print_stop(parser_state *a, dg_stop *message) {
  dump_state* state = a->client_state;
  if(state->print_raw_data)
    print_data(message->data, message->size_bytes);
  printf("stop:\n");
}

void print_synctick(parser_state *a, dg_synctick *message) { PRINT_MESSAGE_PREAMBLE(synctick); }

void print_usercmd(parser_state *a, dg_usercmd *message) {
  dump_state *state = a->client_state;
  PRINT_MESSAGE_PREAMBLE(usercmd); 

  if(state->print_raw_data) {
    print_data(message->data, message->size_bytes);
  }
}

static const char *message_type_name(dg_sendproptype proptype) {

  switch (proptype) {
  case sendproptype_array:
    return "array";
  case sendproptype_datatable:
    return "datatable";
  case sendproptype_float:
    return "float";
  case sendproptype_int:
    return "int";
  case sendproptype_invalid:
    return "invalid";
  case sendproptype_string:
    return "string";
  case sendproptype_vector2:
    return "vector2";
  case sendproptype_vector3:
    return "vector3";
  default:
    break;
  }

  return "";
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

static char *write_str(char *str, char *src, size_t *len) {
  if (*len != 0) {
    *str++ = ' ';
    *len += 1;
  }

  while (*src) {
    *str++ = *src++;
    *len += 1;
  }

  return str;
}

void write_flags(char *writeptr, dg_sendprop *prop) {
  size_t len = 0;

  memcpy(writeptr, "(", 1);
  ++writeptr;

#define WRITE_FLAG(flagname)                                                                       \
  if (prop->flag_##flagname)                                                                       \
    writeptr = write_str(writeptr, #flagname, &len);

  WRITE_FLAG(changesoften);
  WRITE_FLAG(collapsible);
  WRITE_FLAG(coord);
  WRITE_FLAG(coordmp);
  WRITE_FLAG(coordmpint);
  WRITE_FLAG(coordmplp);
  WRITE_FLAG(exclude);
  WRITE_FLAG(insidearray);
  WRITE_FLAG(isvectorelem);
  WRITE_FLAG(normal);
  WRITE_FLAG(noscale);
  WRITE_FLAG(proxyalwaysyes);
  WRITE_FLAG(rounddown);
  WRITE_FLAG(roundup);
  WRITE_FLAG(unsigned);
  WRITE_FLAG(xyze);
  WRITE_FLAG(cellcoord);
  WRITE_FLAG(cellcoordint);
  WRITE_FLAG(cellcoordlp);

  memcpy(writeptr, ")", 2);
}

void print_prop(dg_sendprop *prop) {
  char PROP_TEXT[1024];
  char *writeptr = PROP_TEXT;
  write_flags(writeptr, prop);
  const char *prop_name = get_prop_name(prop);

  if (prop->flag_exclude) {
    printf("\t%s %s (%d) : %s : flags %s\n", message_type_name(prop->proptype), prop_name,
           prop->priority, prop->exclude_name, PROP_TEXT);
  } else if (prop->proptype == sendproptype_datatable) {
    printf("\t%s %s (%d) : %s : flags %s\n", message_type_name(prop->proptype), prop_name,
           prop->priority, prop->dtname, PROP_TEXT);
  } else if (prop->proptype == sendproptype_array) {
    printf("\t%s %s (%d) - %u elements : flags %s\n", message_type_name(prop->proptype), prop_name,
           prop->priority, prop->array_num_elements, PROP_TEXT);
  } else {
    printf("\t%s %s (%d) - %u bit, low %f, high %f : flags %s\n", message_type_name(prop->proptype),
           prop_name, prop->priority, prop->prop_numbits, prop->prop_.low_value,
           prop->prop_.high_value, PROP_TEXT);
  }
}

void print_datatables_parsed(parser_state *a, dg_datatables_parsed *message) {
  dump_state* state = a->client_state;
  uint64_t hash = XXH64(message->_raw_buffer, message->_raw_buffer_bytes, 0);
  printf("Datatables: Tick %d, Slot %d, Hash 0x%lx SizeBytes %lu\n", message->preamble.tick,
         message->preamble.slot, hash, message->_raw_buffer_bytes);

  if(state->print_raw_data) {
    print_data(message->_raw_buffer, message->_raw_buffer_bytes);
  }

  for (size_t i = 0; i < message->sendtable_count; ++i) {
    dg_sendtable *table = message->sendtables + i;
    printf("Sendtable: %s, decoder: %d, props %lu\n", table->name, table->needs_decoder,
           table->prop_count);
    for (size_t u = 0; u < table->prop_count; ++u) {
      dg_sendprop *prop = table->props + u;
      print_prop(prop);
    }
  }

  printf("Server class -> Datatable mappings\n");

  for (size_t i = 0; i < message->serverclass_count; ++i) {
    dg_serverclass *pclass = message->serverclasses + i;
    printf("\tID: %u, %s -> %s\n", pclass->serverclass_id, pclass->serverclass_name,
           pclass->datatable_name);
  }
}

void print_flattened_props(parser_state *state) {
  for (size_t i = 0; i < state->entity_state.serverclass_count; ++i) {
    dg_serverclass_data class_data = state->entity_state.class_datas[i];
    printf("Sendtable %s: flattened, %lu props\n", class_data.dt_name, class_data.prop_count);

    for (size_t u = 0; u < class_data.prop_count; ++u) {
      dg_sendprop *prop = class_data.props + u;
      printf("[%lu]", u);
      print_prop(prop);
    }
  }
}

static const char *update_name(int update_type) {
  switch (update_type) {
  case 0:
    return "Delta";
  case 1:
    return "Leave PVS";
  case 2:
    return "Enter PVS";
  default:
    return "Delete";
  }
}

static void print_inner_prop_value(dg_sendprop *prop, dg_prop_value_inner value);

static void print_int(dg_sendprop *prop, dg_prop_value_inner value) {
  if (prop->flag_unsigned) {
    printf("%u", value.unsigned_val);
  } else {
    printf("%d", value.signed_val);
  }
}

static void print_float(dg_sendprop *prop, dg_prop_value_inner value) {
  float out = dg_prop_to_float(prop, value);
  printf("%f", out);
}

static void print_vec3(dg_sendprop *prop, dg_prop_value_inner value) {
  dg_vector3_value *v3_val = value.v3_val;
  printf("(");
  print_float(prop, v3_val->x);
  printf(", ");
  print_float(prop, v3_val->y);

  if (prop->flag_normal) {
    printf("), sign: %d", v3_val->_sign);
  } else {
    printf(", ");
    print_float(prop, v3_val->z);
    printf(")");
  }
}

static void print_vec2(dg_sendprop *prop, dg_prop_value_inner value) {
  dg_vector2_value *v2_val = value.v2_val;
  printf("(");
  print_float(prop, v2_val->x);
  printf(", ");
  print_float(prop, v2_val->y);
  printf(")");
}

static void print_string(dg_prop_value_inner value) { printf("%s", value.str_val->str); }

static void print_array(dg_sendprop *prop, dg_prop_value_inner value) {
  printf("ARRAY: [");
  dg_array_value *arr_val = value.arr_val;
  if (arr_val->array_size > 0) {
    print_inner_prop_value(prop->array_prop, arr_val->values[0]);
    for (size_t i = 1; i < arr_val->array_size; ++i) {
      printf(", ");
      print_inner_prop_value(prop->array_prop, arr_val->values[i]);
    }
  }
  printf("]");
}

static void print_inner_prop_value(dg_sendprop *prop, dg_prop_value_inner value) {
  if (prop->proptype == sendproptype_int) {
    print_int(prop, value);
  } else if (prop->proptype == sendproptype_float) {
    print_float(prop, value);
  } else if (prop->proptype == sendproptype_string) {
    print_string(value);
  } else if (prop->proptype == sendproptype_vector2) {
    print_vec2(prop, value);
  } else if (prop->proptype == sendproptype_vector3) {
    print_vec3(prop, value);
  } else if (prop->proptype == sendproptype_array) {
    print_array(prop, value);
  } else {
    printf("unparsed value");
  }
}

static void print_prop_value(prop_value *value, dg_serverclass_data* data) {
  dg_sendprop *prop = data->props + value->prop_index;
  const char *prop_name = get_prop_name(prop);
  printf("\t%s = ", prop_name);
  print_inner_prop_value(prop, value->value);
  printf("\n");
}

static void print_packetentities_parsed(parser_state *state,
                                        dg_svc_packetentities_parsed *message) {
  printf("SVC_PacketEntities: %d delta, %lu updates, %lu deletes\n", message->orig->is_delta,
         message->data.ent_updates_count, message->data.explicit_deletes_count);

  for (size_t i = 0; i < message->data.ent_updates_count; ++i) {
    dg_ent_update *update = message->data.ent_updates + i;
    dg_serverclass_data* data = state->entity_state.class_datas + update->datatable_id;

    printf("[%lu] Entity %d, update: %s, props %lu\n", i, update->ent_index,
           update_name(update->update_type), update->prop_value_array_size);
    if (update->update_type == 2) {
      dg_serverclass_data *data = state->entity_state.class_datas + update->datatable_id;
      printf("Handle %d, datatable %s\n", update->handle, data->dt_name);
    }

    if (update->prop_value_array_size > 0) {
      for (size_t u = 0; u < update->prop_value_array_size; ++u) {
        prop_value *value = update->prop_value_array + u;
        print_prop_value(value, data);
      }
    }
  }
}

void handle_version(parser_state *a, dg_demver_data message) {
  dump_state *state = a->client_state;
  state->version_data = message;
}

void print_help() {
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
  settings->demo_version_handler = handle_version;
  if(check_if_enabled(arg, "flattened"))
    settings->flattened_props_handler = print_flattened_props;
  if(check_if_enabled(arg, "datatables"))
    settings->datatables_parsed_handler = print_datatables_parsed;
  if(check_if_enabled(arg, "consolecmd"))
    settings->consolecmd_handler = print_consolecmd;
  if(check_if_enabled(arg, "customdata"))
    settings->customdata_handler = print_customdata;
  if(check_if_enabled(arg, "header"))
    settings->header_handler = print_header;
  if(check_if_enabled(arg, "packet"))
    settings->packet_parsed_handler = print_packet;
  if(check_if_enabled(arg, "stop"))
    settings->stop_handler = print_stop;
  if(check_if_enabled(arg, "stringtables"))
    settings->stringtables_parsed_handler = print_stringtables_parsed;
  if(check_if_enabled(arg, "synctick"))
    settings->synctick_handler = print_synctick;
  if(check_if_enabled(arg, "usercmd"))
    settings->usercmd_handler = print_usercmd;
  if(check_if_enabled(arg, "entupdates"))
    settings->packetentities_parsed_handler = print_packetentities_parsed;
  if(check_if_enabled(arg, "raw"))
    state->print_raw_data = true;
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    printf("Usage: demodump <filepath>\n");
    return 0;
  }

  dg_settings settings;
  dg_settings_init(&settings);
  dump_state dump;
  memset(&dump, 0, sizeof(dump_state));
  settings.client_state = &dump;
  bool settings_init = false;


  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    if (strcmp(arg, "--help") == 0) {
      print_help();
      return 0;
    }
    if (strcmp(arg, "--filter") == 0) {
      if (i == argc - 2) {
        fprintf(stderr, "No argument provided for --filter\n");
        return 1;
      } else {
        get_settings(argv[i+1], &settings);
        settings_init = true;
      }
    }
  }

  if(!settings_init) {
    get_settings(NULL, &settings);
  }

  const char *filepath = argv[argc - 1];

  dg_parse_result result = dg_parse_file(&settings, filepath);

  if (result.error) {
    printf("%s\n", result.error_message);
  }

  return 0;
}