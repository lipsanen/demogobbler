#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "demogobbler_bitstream.h"
#include "demogobbler_floats.h"

// clang-format off
#define DEMOGOBBLER_MACRO_ALL_MESSAGES(macro) \
  macro(net_nop)  \
  macro(net_disconnect) \
  macro(net_file) \
  macro(net_tick) \
  macro(net_stringcmd) \
  macro(net_setconvar) \
  macro(net_signonstate) \
  macro(net_splitscreen_user) \
  macro(svc_print) \
  macro(svc_serverinfo) \
  macro(svc_sendtable) \
  macro(svc_classinfo) \
  macro(svc_setpause) \
  macro(svc_create_stringtable) \
  macro(svc_update_stringtable) \
  macro(svc_voice_init) \
  macro(svc_voice_data) \
  macro(svc_sounds) \
  macro(svc_setview) \
  macro(svc_fixangle) \
  macro(svc_crosshair_angle) \
  macro(svc_bsp_decal) \
  macro(svc_user_message) \
  macro(svc_entity_message) \
  macro(svc_game_event) \
  macro(svc_packet_entities) \
  macro(svc_splitscreen) \
  macro(svc_temp_entities) \
  macro(svc_prefetch) \
  macro(svc_menu) \
  macro(svc_game_event_list) \
  macro(svc_get_cvar_value) \
  macro(svc_cmd_key_values) \
  macro(svc_paintmap_data)
// clang-format on

#define DEMOGOBBLER_DECLARE_ENUMS(x) x,

enum net_message_type { DEMOGOBBLER_MACRO_ALL_MESSAGES(DEMOGOBBLER_DECLARE_ENUMS) svc_invalid };
typedef enum net_message_type net_message_type;

#undef DEMOGOBBLER_DECLARE_ENUMS

struct demogobbler_net_nop {
  uint8_t _empty;
};

struct demogobbler_net_disconnect {
  const char *text; // Null terminated
};

struct demogobbler_net_file {
  const char *filename; // Null terminated
  uint32_t transfer_id; // 32 bit
  unsigned int file_requested; // 2 bits
};

struct demogobbler_net_splitscreen_user {
  bool unk;
};

struct demogobbler_net_tick {
  int32_t tick;
  uint16_t host_frame_time;
  uint16_t host_frame_time_std_dev;
};

struct demogobbler_net_stringcmd {
  const char *command;
};

struct demogobbler_net_setconvar_convar {
  const char *name;
  const char *value;
};

struct demogobbler_net_setconvar {
  uint8_t count;
  struct demogobbler_net_setconvar_convar *convars;
};

struct demogobbler_net_signonstate {
  uint8_t signon_state;
  uint32_t spawn_count;
  uint32_t NE_num_server_players;
  uint32_t NE_map_name_length;
  bitstream NE_player_network_ids; 
  const char *NE_map_name;
};

struct demogobbler_svc_serverinfo {
  int16_t network_protocol;
  bool is_hltv;
  bool is_dedicated;
  uint32_t server_count;
  int32_t client_crc;
  int32_t stringtable_crc; // protocol 24 only
  int16_t max_classes;
  char map_md5[16];
  int32_t map_crc;
  uint8_t player_count;
  uint8_t max_clients;
  float tick_interval;
  char *game_dir;
  char *map_name;
  char *sky_name;
  char *host_name;
  uint8_t platform;

  // L4D specific
  char* mission_name;
  char* mutation_name;
  bool unk_l4d_bit;

  // Network protocol 24
  bool has_replay;
};

struct demogobbler_svc_sendtable {
  bool needs_decoder;
  uint16_t length;
  bitstream data;
};

struct demogobbler_svc_classinfo_serverclass {
  int32_t class_id;
  const char *class_name;
  const char *datatable_name;
};

struct demogobbler_svc_classinfo {
  uint16_t length;
  bool create_on_client;
  struct demogobbler_svc_classinfo_serverclass *server_classes;
};

struct demogobbler_svc_setpause {
  bool paused;
};

struct demogobbler_svc_create_stringtable {
  const char *name;
  uint16_t max_entries;
  uint16_t num_entries;
  uint32_t data_length;
  bool user_data_fixed_size : 1;
  unsigned int user_data_size : 12;
  unsigned int user_data_size_bits : 4;
  unsigned int flags : 2;
  bitstream data;
};

struct demogobbler_svc_update_stringtable {
  unsigned int table_id : 5; // ID of the table
  uint32_t changed_entries;
  unsigned int data_length : 20; // Length of the data
  bool exists : 1;
  bitstream data;
};

struct demogobbler_svc_voice_init {
  const char *codec; // Null-terminated
  uint8_t quality; // byte
  float unk; // only read if quality == 255
};

struct demogobbler_svc_voice_data {
  uint8_t client;
  uint8_t proximity;
  uint16_t length;
  bitstream data;
};

struct demogobbler_svc_print {
  const char *message;
};

struct demogobbler_svc_sounds {
  bool reliable_sound;
  uint8_t sounds;
  uint16_t length;
  bitstream data;
  // TODO: SoundInfo stuff for protocol 3
};

struct demogobbler_svc_setview {
  int32_t entity_index;
};

struct demogobbler_svc_fixangle {
  bool relative;
  bitangle_vector angle;
};

struct demogobbler_svc_crosshair_angle {
  bitangle_vector angle;
};

struct demogobbler_svc_bsp_decal {
  bitcoord_vector pos;
  unsigned int decal_texture_index : 9;
  bool index_bool : 1;
  unsigned int entity_index : 11;
  unsigned int model_index : 11;
  bool lowpriority : 1;
};

struct demogobbler_svc_user_message {
  uint8_t msg_type; // type of the user message
  unsigned int length; // specifies the length for the void* buffer in bits
  bitstream data;
};

struct demogobbler_svc_entity_message {
  unsigned int entity_index;
  unsigned int class_id;
  unsigned int length;
  bitstream data;
};

struct demogobbler_svc_game_event {
  unsigned int length;
  bitstream data;
};

struct demogobbler_svc_packet_entities {
  unsigned int max_entries;
  bool is_delta;
  int32_t delta_from;
  bool base_line;
  unsigned int updated_entries;
  unsigned int data_length;
  bool update_baseline;
  bitstream data;
};

struct demogobbler_svc_splitscreen {
  bool remove_user : 1;
  unsigned int data_length : 11;
  bitstream data;
};

struct demogobbler_svc_temp_entities {
  uint8_t num_entries;
  unsigned int data_length;
  bitstream data;
};

struct demogobbler_svc_prefetch {
  unsigned int sound_index;
};

struct demogobbler_svc_menu {
  unsigned int menu_type;
  uint32_t data_length;
  bitstream data;
};

struct demogobbler_game_event_descriptor_key {
  const char *key_value;
  unsigned int type : 3;
};

struct demogobbler_game_event_descriptor {
  unsigned int event_id : 9;
  const char *name;
  size_t key_count;
  struct demogobbler_game_event_descriptor_key *keys;
};

struct demogobbler_svc_game_event_list {
  unsigned int events : 9;
  unsigned int length : 20;
  bitstream data; // Add game event descriptor array here later on
};

struct demogobbler_svc_get_cvar_value {
  uint32_t cookie;
  const char *cvar_name;
};

struct demogobbler_svc_cmd_key_values {
  uint32_t data_length;
  bitstream data;
};

struct demogobbler_svc_paintmap_data {
  uint32_t data_length;
  bitstream data;
};

#define DECLARE_MESSAGE_IN_UNION(message) struct demogobbler_##message message_##message;

struct demogobbler_packet_net_message {
  net_message_type mtype;
  unsigned int _mtype : 6;
  bool last_message : 1;
#ifdef DEBUG
  uint32_t offset;
#endif

  union {
    DEMOGOBBLER_MACRO_ALL_MESSAGES(DECLARE_MESSAGE_IN_UNION)
  };
};

typedef struct demogobbler_packet_net_message packet_net_message;

enum demogobbler_game {
  csgo, l4d, l4d2, portal2, orangebox, steampipe
};

struct demo_version_data {
  enum demogobbler_game game : 3;
  unsigned int netmessage_type_bits : 3;
  unsigned int demo_protocol : 3;
  unsigned int cmdinfo_size : 3;
  unsigned int net_file_bits : 2;
  unsigned int stringtable_flags_bits : 2;
  unsigned int stringtable_userdata_size_bits : 5;
  unsigned int svc_user_message_bits : 4;
  unsigned int svc_prefetch_bits : 4;
  unsigned int model_index_bits : 4;
  unsigned int datatable_propcount_bits : 4;
  unsigned int sendprop_flag_bits : 5;
  unsigned int sendprop_numbits_for_numbits : 4;
  unsigned int has_slot_in_preamble : 1;
  unsigned int has_nettick_times : 1;
  unsigned int l4d2_version_finalized : 1;
  unsigned int svc_update_stringtable_table_id_bits : 4;
  net_message_type* netmessage_array;
  unsigned int netmessage_count;
  unsigned int network_protocol;
  unsigned int l4d2_version;
};

typedef struct demo_version_data demo_version_data;


#ifdef __cplusplus
}
#endif
