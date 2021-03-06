#include "version_utils.h"
#include "utils.h"
#include <string.h>


// clang-format off
// protocol 2
static net_message_type protocol_2_messages[] =
{
  svc_invalid,
  net_nop,
  net_disconnect,
  svc_invalid, // svc_event
  svc_invalid, // svc_version
  svc_setview, 
  svc_sounds,
  svc_invalid, // svc_time
  svc_print,
  svc_invalid, // svc_stufftext
  svc_crosshair_angle, // svc_setangle
  svc_serverinfo,
  svc_invalid, // svc_lightstyles
  svc_invalid, // svc_updateuserinfo
  svc_invalid, // svc_event_reliable
  svc_invalid, // svc_clientdata
  svc_invalid, // svc_stopsound
  svc_create_stringtable,
  svc_update_stringtable,
  svc_entity_message,
  svc_invalid, // svc_spawnbaseline
  svc_bsp_decal, 
  svc_setpause,
  svc_invalid, // svc_signonnum
  svc_invalid, // svc_centerprint
  svc_invalid, // svc_spawnstaticsound
  svc_game_event
};

// steampipe and orangebox
static net_message_type old_protocol_messages[] =
{
  net_nop,
  net_disconnect,
  net_file,
  net_tick,
  net_stringcmd,
  net_setconvar,
  net_signonstate,
  svc_print,
  svc_serverinfo,
  svc_sendtable,
  svc_classinfo,
  svc_setpause,
  svc_create_stringtable,
  svc_update_stringtable,
  svc_voice_init,
  svc_voice_data,
  svc_invalid, // svc_HLTV
  svc_sounds,
  svc_setview,
  svc_fixangle,
  svc_crosshair_angle,
  svc_bsp_decal,
  svc_invalid, // svc_terrainmod
  svc_user_message,
  svc_entity_message,
  svc_game_event,
  svc_packet_entities,
  svc_temp_entities,
  svc_prefetch,
  svc_menu,
  svc_game_event_list,
  svc_get_cvar_value,
  svc_cmd_key_values // steampipe only
};

static net_message_type new_protocol_messages[] =
{
  net_nop,
  net_disconnect,
  net_file,
  net_splitscreen_user,
  net_tick,
  net_stringcmd,
  net_setconvar,
  net_signonstate,
  svc_serverinfo,
  svc_sendtable,
  svc_classinfo,
  svc_setpause,
  svc_create_stringtable,
  svc_update_stringtable,
  svc_voice_init,
  svc_voice_data,
  svc_print,
  svc_sounds,
  svc_setview,
  svc_fixangle,
  svc_crosshair_angle,
  svc_bsp_decal,
  svc_splitscreen,
  svc_user_message,
  svc_entity_message,
  svc_game_event,
  svc_packet_entities,
  svc_temp_entities,
  svc_prefetch,
  svc_menu,
  svc_game_event_list,
  svc_get_cvar_value,
  svc_cmd_key_values, // Not in old L4D
  svc_paintmap_data // Added in protocol 4
};
// clang-format on

static void get_net_message_array(demo_version_data* version) {
  if(version->demo_protocol == 1) {
    version->netmessage_array = protocol_2_messages;
    version->netmessage_count = ARRAYSIZE(protocol_2_messages);
  }
  else if(version->demo_protocol <= 3) {
    version->netmessage_array = old_protocol_messages;
    version->netmessage_count = ARRAYSIZE(old_protocol_messages);
  }
  else {
    version->netmessage_array = new_protocol_messages;
    version->netmessage_count = ARRAYSIZE(new_protocol_messages);
  }
}

static void get_net_message_bits(demo_version_data* version) {
  if(version->network_protocol <= 14) {
    version->netmessage_type_bits = 5;
  } 
  else if(version->game == l4d && version->network_protocol <= 37) {
    version->netmessage_type_bits = 5;
  }
  else {
    version->netmessage_type_bits = 6;
  }
}

static void get_preamble_info(demo_version_data* version) {
  if (version->game == portal2 || version->game == csgo || version->game == l4d ||
      version->game == l4d2) {
    version->has_slot_in_preamble = true;
  } else {
   version->has_slot_in_preamble =false;
  }
}

static void get_cmdinfo_size(demo_version_data* version) {
  if (version->game == l4d || version->game == l4d2) {
    version->cmdinfo_size = 4;
  } else if (version->game == portal2 || version->game == csgo) {
    version->cmdinfo_size = 2;
  } else {
    version->cmdinfo_size = 1;
  }
}


static void get_net_file_bits(demo_version_data* version) {
  if(version->demo_protocol <= 3) {
    version->net_file_bits = 1;
  } else {
    version->net_file_bits = 2;
  }
}

static void get_has_nettick_times(demo_version_data* version) {
  if(version->network_protocol < 14) {
    version->has_nettick_times = false;
  } else {
    version->has_nettick_times = true;
  }
}


static void get_stringtable_flags_bits(demo_version_data* version) {
  if(version->demo_protocol >= 4) {
    version->stringtable_flags_bits = 2;
  }
  else {
    version->stringtable_flags_bits = 1;
  }
}

static void get_stringtable_userdata_size_bits(demo_version_data* version) {
  if(version->game == l4d2) {
    version->stringtable_userdata_size_bits = 21;
  }
  else {
    version->stringtable_userdata_size_bits = 20;
  }
}

static void get_svc_user_message_bits(demo_version_data* version) {
  if(version->game == l4d) {
    version->svc_user_message_bits = 11;
  }
  else if (version->demo_protocol >= 4) {
    version->svc_user_message_bits = 12;
  }
  else {
    version->svc_user_message_bits = 11;
  }
}

static void get_svc_prefetch_bits(demo_version_data* version) {
  if(version->game == l4d2 && version->l4d_version >= 2091) {
    version->svc_prefetch_bits = 15;
  }
  else if(version->game == l4d2) {
    version->svc_prefetch_bits = 14;
  }
  else {
    version->svc_prefetch_bits = 13;
  }
}

static void get_model_index_bits(demo_version_data* version) {
  if(version->game == l4d2 && version->l4d_version >= 2203) {
    version->model_index_bits = 12;
  }
  else {
    version->model_index_bits = 11;
  }
}

struct version_pair {
  const char *game_directory;
  enum demogobbler_game version;
};

typedef struct version_pair version_pair;

static version_pair versions[] = {
    {"aperturetag", portal2},
    {"portal2", portal2},
    {"portalreloaded", portal2},
    {"portal_stories", portal2},
    {"TWTM", portal2},
    {"csgo", csgo},
    {"left4dead2", l4d2},
    {"left4dead", l4d}};

demo_version_data get_demo_version(demogobbler_header *header) {
  demo_version_data version;
  version.game = orangebox;
  version.demo_protocol = header->demo_protocol;
  version.network_protocol = header->net_protocol;
  version.l4d_version = 2147;

  if(version.demo_protocol <= 3) {
    if(version.network_protocol >= 24) {
      version.game = steampipe;
    }
    else {
      version.game = orangebox;
    }
  }
  else {
    for (size_t i = 0; i < ARRAYSIZE(versions); ++i) {
      if (strcmp(header->game_directory, versions[i].game_directory) == 0) {
        version.game = versions[i].version;
        break;
      }
    }
  }

  get_cmdinfo_size(&version);
  get_preamble_info(&version);
  get_net_message_array(&version);
  get_net_message_bits(&version);
  get_net_file_bits(&version);
  get_has_nettick_times(&version);
  get_stringtable_flags_bits(&version);
  get_stringtable_userdata_size_bits(&version);
  get_svc_user_message_bits(&version);
  get_svc_prefetch_bits(&version);
  get_model_index_bits(&version);

  return version;
}
