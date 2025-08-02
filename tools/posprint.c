#include "demogobbler.h"
#include "demogobbler/conversions.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static const char *get_prop_name(dg_sendprop *prop) {
  static char BUFFER[1024];

  if (prop->proptype != sendproptype_datatable && prop->baseclass) {
    snprintf(BUFFER, sizeof(BUFFER), "%s.%s", prop->baseclass->name, prop->name);
    return BUFFER;
  } else {
    return prop->name;
  }
}

typedef struct posprint_s {
    float pos[3];
} PosPrint;

static void handle_svcpacketentities_parsed(parser_state *state, dg_svc_packetentities_parsed *message) {
  PosPrint* pp = (PosPrint*)state->client_state;
  for(size_t i=0; i < message->data.ent_updates_count; ++i) {
    dg_ent_update* update = message->data.ent_updates + i;
    dg_serverclass_data* data = state->entity_state.class_datas + update->datatable_id;

    if(update->ent_index == 0)
      continue;
    else if(update->ent_index > 1)
      break;

    for(size_t prop_index=0; prop_index < update->prop_value_array_size; ++prop_index) {
      prop_value value = update->prop_value_array[prop_index];
      dg_sendprop* prop = data->props + value.prop_index;
      const char* prop_name = get_prop_name(prop);
    
      if(strstr(prop_name, "m_vecOrigin") != NULL)
      { 
        dg_vector3_value *v3_val = value.value.v3_val;
        float x = dg_prop_to_float(prop, v3_val->x);
        float y = dg_prop_to_float(prop, v3_val->y);
        float z = dg_prop_to_float(prop, v3_val->z);
        pp->pos[0] = x;    
        pp->pos[1] = y;    
        pp->pos[2] = z;    
      }
    }
  }
}

static void handle_packet(parser_state* state, dg_packet* packet)
{
    if(packet->preamble.converted_type == dg_type_packet)
    {
        PosPrint* pp = (PosPrint*)state->client_state;
        printf("packet tick %d, pos (%f, %f, %f)\n", packet->preamble.tick, 
             pp->pos[0], pp->pos[1], pp->pos[2]);
    }
    else if (packet->preamble.converted_type == dg_type_signon) {  
        PosPrint* pp = (PosPrint*)state->client_state;
        printf("signon tick %d, pos (%f, %f, %f)\n", packet->preamble.tick, 
             pp->pos[0], pp->pos[1], pp->pos[2]);
    }
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    printf("Usage: posprint <filepath>\n");
    return 0;
  }

  PosPrint pp;
  memset(&pp, 0, sizeof(pp));
  pp.pos[0] = pp.pos[1] = pp.pos[2] = 9999.0f;

  dg_settings settings;
  dg_settings_init(&settings);
  settings.packetentities_parsed_handler = handle_svcpacketentities_parsed;
  settings.packet_handler = handle_packet; 
  settings.client_state = &pp;

  dg_parse_file(&settings, argv[1]);

  return 0;
}
