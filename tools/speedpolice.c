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

typedef struct speedpolice_s {
    float vel[3];
    int currentTick;
    int previousJump;
    float speedlimit;
} Speedpolice;

static void handle_svcpacketentities_parsed(parser_state *state, dg_svc_packetentities_parsed *message) {
  Speedpolice* police = (Speedpolice*)state->client_state;
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
    
      if(strstr(prop_name, "m_vecVelocity[0]") != NULL)
      { 
        float out = dg_prop_to_float(prop, value.value);
        police->vel[0] = out;    
      }
      else if(strstr(get_prop_name(prop), "m_vecVelocity[1]") != NULL)
      {
        float out = dg_prop_to_float(prop, value.value);
        police->vel[1] = out;
      }
      else if(strstr(get_prop_name(prop), "m_vecVelocity[2]")!= NULL)
      {
        float out = dg_prop_to_float(prop, value.value);
        police->vel[2] = out;
      }
    }
  }
}

static void handle_consolecmd(parser_state* state, dg_consolecmd* cmd)
{
    Speedpolice* police = (Speedpolice*)state->client_state;
    if(strstr(cmd->data, "+jump") != NULL && (cmd->preamble.tick - police->previousJump) > 1)
    {
        police->previousJump = cmd->preamble.tick;
    }
}

static float speed(float* vel)
{
    return sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
}

static void handle_packet(parser_state* state, dg_packet* packet)
{
    Speedpolice* police = (Speedpolice*)state->client_state;
    float xyspeed = speed(police->vel);
    if(police->currentTick - police->previousJump == 1 && xyspeed > police->speedlimit && police->vel[2] > 146)
    {
        printf("tick %d, horizontal speed %f, speed (%f, %f, %f)\n", police->currentTick, 
            xyspeed, police->vel[0], police->vel[1], police->vel[2]);
    }
    police->currentTick = packet->preamble.tick;
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    printf("Usage: speedpolice <filepath> [speedLimit]\n");
    return 0;
  }

  Speedpolice police;
  memset(&police, 0, sizeof(police));
  if(argc > 2)
  {
    police.speedlimit = atof(argv[2]);
  }
  else
  {
    police.speedlimit = 225.0f;
  }

  dg_settings settings;
  dg_settings_init(&settings);
  settings.packetentities_parsed_handler = handle_svcpacketentities_parsed;
  settings.consolecmd_handler = handle_consolecmd;
  settings.packet_handler = handle_packet; 
  settings.client_state = &police;

  dg_parse_file(&settings, argv[1]);

  return 0;
}
