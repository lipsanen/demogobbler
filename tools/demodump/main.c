#include "demogobbler.h"
#include "demogobbler_conversions.h"
#include "demogobbler_datatable_types.h"
#include <stdio.h>
#include <string.h>

void print_header(parser_state *a, demogobbler_header *header) {
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
  demo_version_data version_data;
};

typedef struct dump_state dump_state;

#define PRINT_MESSAGE_PREAMBLE(name)                                                               \
  printf(#name ", Tick %d, Slot %d\n", message->preamble.tick, message->preamble.slot);

void print_consolecmd(parser_state *a, demogobbler_consolecmd *message) {
  PRINT_MESSAGE_PREAMBLE(consolecmd);
  printf("\t%s\n", message->data);
}

void print_customdata(parser_state *a, demogobbler_customdata *message) {
  PRINT_MESSAGE_PREAMBLE(customdata);
}

void print_packet(parser_state *a, demogobbler_packet *message) {
  dump_state *state = a->client_state;
  if (message->preamble.type == demogobbler_type_signon) {
    printf("Signon, Tick %d, Slot %d\n", message->preamble.tick, message->preamble.slot);
  } else {
    printf("Packet, Tick %d, Slot %d\n", message->preamble.tick, message->preamble.slot);
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
  printf("Messages:\n");
}

void print_stringtables(parser_state *a, demogobbler_stringtables *message) {
  PRINT_MESSAGE_PREAMBLE(stringtables);
}

void print_stop(parser_state *a, demogobbler_stop *message) { printf("stop:\n"); }

void print_synctick(parser_state *a, demogobbler_synctick *message) {
  PRINT_MESSAGE_PREAMBLE(synctick);
}

void print_usercmd(parser_state *a, demogobbler_usercmd *message) {
  PRINT_MESSAGE_PREAMBLE(usercmd);
}

void print_netmessage(parser_state *a, packet_net_message *message) {}

static const char *message_type_name(demogobbler_sendproptype proptype) {

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

static const char *get_prop_name(demogobbler_sendprop *prop) {
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

void write_flags(char *writeptr, demogobbler_sendprop *prop) {
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

void print_prop(demogobbler_sendprop *prop) {
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

void print_datatables_parsed(parser_state *a, demogobbler_datatables_parsed *message) {
  PRINT_MESSAGE_PREAMBLE(datatables);

  for (size_t i = 0; i < message->sendtable_count; ++i) {
    demogobbler_sendtable *table = message->sendtables + i;
    printf("Sendtable: %s, decoder: %d, props %lu\n", table->name, table->needs_decoder,
           table->prop_count);
    for (size_t u = 0; u < table->prop_count; ++u) {
      demogobbler_sendprop *prop = table->props + u;
      print_prop(prop);
    }
  }

  printf("Server class -> Datatable mappings\n");

  for (size_t i = 0; i < message->serverclass_count; ++i) {
    demogobbler_serverclass *pclass = message->serverclasses + i;
    printf("\tID: %u, %s -> %s\n", pclass->serverclass_id, pclass->serverclass_name,
           pclass->datatable_name);
  }
}

void print_flattened_props(parser_state *state) {
  for (size_t i = 0; i < state->entity_state.serverclass_count; ++i) {
    serverclass_data class_data = state->entity_state.class_datas[i];
    printf("Sendtable %s, flattened, %lu props\n", class_data.dt_name,
           class_data.prop_count);

    for (size_t u = 0; u < class_data.prop_count; ++u) {
      demogobbler_sendprop *prop = class_data.props + u;
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

static void print_inner_prop_value(demogobbler_sendprop *prop, prop_value_inner value);

static void print_int(demogobbler_sendprop *prop, prop_value_inner value) {
  if (prop->flag_unsigned) {
    printf("%u", value.unsigned_val);
  } else {
    printf("%d", value.signed_val);
  }
}

static void print_float(demogobbler_sendprop *prop, prop_value_inner value) {
  float out = demogobbler_prop_to_float(prop, value);
  printf("%f", out);
}

static void print_vec3(demogobbler_sendprop *prop, prop_value_inner value) {
  vector3_value *v3_val = value.v3_val;
  printf("(");
  print_float(prop, v3_val->x);
  printf(", ");
  print_float(prop, v3_val->y);

  if (prop->flag_normal) {
    printf("), sign: %d", v3_val->sign);
  } else {
    printf(", ");
    print_float(prop, v3_val->y);
    printf(")");
  }
}

static void print_vec2(demogobbler_sendprop *prop, prop_value_inner value) {
  vector2_value *v2_val = value.v2_val;
  printf("(");
  print_float(prop, v2_val->x);
  printf(", ");
  print_float(prop, v2_val->y);
  printf(")");
}

static void print_string(prop_value_inner value) { printf("%s", value.str_val->str); }

static void print_array(demogobbler_sendprop *prop, prop_value_inner value) {
  printf("ARRAY: [");
  array_value *arr_val = value.arr_val;
  if (arr_val->array_size > 0) {
    print_inner_prop_value(prop->array_prop, arr_val->values[0]);
    for (size_t i = 1; i < arr_val->array_size; ++i) {
      printf(", ");
      print_inner_prop_value(prop->array_prop, arr_val->values[i]);
    }
  }
  printf("]");
}

static void print_inner_prop_value(demogobbler_sendprop *prop, prop_value_inner value) {
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

static void print_prop_value(prop_value *value) {
  demogobbler_sendprop *prop = value->prop;
  printf("\t%s = ", value->prop->name);
  print_inner_prop_value(prop, value->value);
  printf("\n");
}

static void print_packetentities_parsed(parser_state *state, svc_packetentities_parsed *message) {
  printf("SVC_PacketEntities: %d delta, %lu updates, %lu deletes\n", message->orig->is_delta,
         message->data.ent_updates_count, message->data.explicit_deletes_count);

  for (size_t i = 0; i < message->data.ent_updates_count; ++i) {
    ent_update *update = message->data.ent_updates + i;

    printf("[%lu] Entity %d, update: %s, props %lu\n", i, update->ent_index,
           update_name(update->update_type), update->prop_value_array_size);
    if (update->update_type == 2) {
      serverclass_data *data = state->entity_state.class_datas + update->ent->datatable_id;
      printf("Handle %d, datatable %s\n", update->ent->handle, data->dt_name);
    }

    if (update->prop_value_array_size > 0) {
      for (size_t u = 0; u < update->prop_value_array_size; ++u) {
        prop_value *value = update->prop_value_array + u;
        print_prop_value(value);
      }
    }
  }
}

void handle_version(parser_state *a, demo_version_data message) {
  dump_state *state = a->client_state;
  state->version_data = message;
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    printf("Usage: demodump <filepath>\n");
    return 0;
  }

  dump_state dump;
  memset(&dump, 0, sizeof(dump_state));
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.flattened_props_handler = print_flattened_props;
  settings.datatables_parsed_handler = print_datatables_parsed;
  settings.store_ents = true;
  settings.consolecmd_handler = print_consolecmd;
  settings.customdata_handler = print_customdata;
  settings.header_handler = print_header;
  settings.packet_handler = print_packet;
  settings.stop_handler = print_stop;
  settings.stringtables_handler = print_stringtables;
  settings.synctick_handler = print_synctick;
  settings.usercmd_handler = print_usercmd;
  settings.packet_net_message_handler = print_netmessage;
  settings.packetentities_parsed_handler = print_packetentities_parsed;
  settings.client_state = &dump;

  demogobbler_parse_result result = demogobbler_parse_file(&settings, argv[1]);

  if (result.error) {
    printf("%s\n", result.error_message);
  }

  return 0;
}