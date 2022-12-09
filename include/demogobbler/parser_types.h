#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler/bitstream.h"
#include "demogobbler/floats.h"

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

struct dg_net_nop {
  uint8_t _empty;
};

struct dg_net_disconnect {
  const char *text; // Null terminated
};

struct dg_net_file {
  const char *filename;    // Null terminated
  uint32_t transfer_id;    // 32 bit
  uint32_t file_requested; // 2 bits
};

struct dg_net_splitscreen_user {
  bool unk;
};

struct dg_net_tick {
  int32_t tick;
  uint16_t host_frame_time;
  uint16_t host_frame_time_std_dev;
};

struct dg_net_stringcmd {
  const char *command;
};

struct dg_net_setconvar_convar {
  const char *name;
  const char *value;
};

struct dg_net_setconvar {
  uint8_t count;
  struct dg_net_setconvar_convar *convars;
};

struct dg_net_signonstate {
  const char *NE_map_name;
  uint32_t spawn_count;
  uint32_t NE_num_server_players;
  uint32_t NE_map_name_length;
  uint8_t signon_state;
  dg_bitstream NE_player_network_ids;
};

struct dg_svc_serverinfo {
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
  char *mission_name;
  char *mutation_name;
  bool unk_l4d_bit;

  // Network protocol 24
  bool has_replay;
};

struct dg_svc_sendtable {
  bool needs_decoder;
  uint16_t length;
  dg_bitstream data;
};

struct dg_svc_classinfo_serverclass {
  int32_t class_id;
  const char *class_name;
  const char *datatable_name;
};

struct dg_svc_classinfo {
  uint16_t length;
  bool create_on_client;
  struct dg_svc_classinfo_serverclass *server_classes;
};

struct dg_svc_setpause {
  bool paused;
};

struct dg_svc_create_stringtable {
  const char *name;
  uint32_t data_length;
  uint32_t user_data_size;
  uint32_t user_data_size_bits;
  uint32_t flags;
  uint16_t max_entries;
  uint16_t num_entries;
  bool user_data_fixed_size;
  dg_bitstream data;
};

struct dg_svc_update_stringtable {
  uint32_t table_id; // ID of the table
  uint32_t changed_entries;
  uint32_t data_length; // Length of the data
  bool exists;
  dg_bitstream data;
};

struct dg_svc_voice_init {
  const char *codec; // Null-terminated
  uint8_t quality;   // byte
  float unk;         // only read if quality == 255
};

struct dg_svc_voice_data {
  uint8_t client;
  uint8_t proximity;
  uint16_t length;
  dg_bitstream data;
};

struct dg_svc_print {
  const char *message;
};

struct dg_svc_sounds {
  bool reliable_sound;
  uint8_t sounds;
  uint16_t length;
  dg_bitstream data;
  // TODO: SoundInfo stuff for protocol 3
};

struct dg_svc_setview {
  int32_t entity_index;
};

struct dg_svc_fixangle {
  bool relative;
  dg_bitangle_vector angle;
};

struct dg_svc_crosshair_angle {
  dg_bitangle_vector angle;
};

struct dg_svc_bsp_decal {
  dg_bitcoord_vector pos;
  uint32_t decal_texture_index;
  uint32_t entity_index;
  uint32_t model_index;
  bool index_bool;
  bool lowpriority;
};

struct dg_svc_user_message {
  uint8_t msg_type; // type of the user message
  uint32_t length;  // specifies the length for the void* buffer in bits
  dg_bitstream data;
};

struct dg_svc_entity_message {
  uint32_t entity_index;
  uint32_t class_id;
  uint32_t length;
  dg_bitstream data;
};

struct dg_svc_game_event {
  uint32_t length;
  dg_bitstream data;
};

struct dg_svc_packet_entities {
  uint32_t max_entries;
  int32_t delta_from;
  uint32_t updated_entries;
  uint32_t data_length;
  bool update_baseline;
  bool base_line;
  bool is_delta;
  dg_bitstream data;
  dg_svc_packetentities_parsed* parsed;
};

struct dg_svc_splitscreen {
  uint32_t data_length;
  bool remove_user;
  dg_bitstream data;
};

struct dg_svc_temp_entities {
  uint8_t num_entries;
  uint32_t data_length;
  dg_bitstream data;
};

struct dg_svc_prefetch {
  uint32_t sound_index;
};

struct dg_svc_menu {
  uint32_t menu_type;
  uint32_t data_length;
  dg_bitstream data;
};

struct dg_game_event_descriptor_key {
  const char *key_value;
  uint32_t type;
};

struct dg_game_event_descriptor {
  uint32_t event_id;
  const char *name;
  size_t key_count;
  struct dg_game_event_descriptor_key *keys;
};

struct dg_svc_game_event_list {
  uint32_t events;
  uint32_t length;
  dg_bitstream data; // Add game event descriptor array here later on
};

struct dg_svc_get_cvar_value {
  uint32_t cookie;
  const char *cvar_name;
};

struct dg_svc_cmd_key_values {
  uint32_t data_length;
  dg_bitstream data;
};

struct dg_svc_paintmap_data {
  uint32_t data_length;
  dg_bitstream data;
};

#define DECLARE_MESSAGE_IN_UNION(message) struct dg_##message message_##message

struct dg_packet_net_message {
  net_message_type mtype;
#ifdef DEBUG
  uint32_t offset;
#endif

  union {
    DECLARE_MESSAGE_IN_UNION(net_nop);
    DECLARE_MESSAGE_IN_UNION(net_disconnect);
    DECLARE_MESSAGE_IN_UNION(net_file);
    DECLARE_MESSAGE_IN_UNION(net_tick);
    DECLARE_MESSAGE_IN_UNION(net_stringcmd);
    DECLARE_MESSAGE_IN_UNION(net_setconvar);
    DECLARE_MESSAGE_IN_UNION(net_signonstate);
    DECLARE_MESSAGE_IN_UNION(net_splitscreen_user);
    DECLARE_MESSAGE_IN_UNION(svc_print);
    struct dg_svc_serverinfo *message_svc_serverinfo;
    DECLARE_MESSAGE_IN_UNION(svc_sendtable);
    DECLARE_MESSAGE_IN_UNION(svc_classinfo);
    DECLARE_MESSAGE_IN_UNION(svc_setpause);
    DECLARE_MESSAGE_IN_UNION(svc_create_stringtable);
    DECLARE_MESSAGE_IN_UNION(svc_update_stringtable);
    DECLARE_MESSAGE_IN_UNION(svc_voice_init);
    DECLARE_MESSAGE_IN_UNION(svc_voice_data);
    DECLARE_MESSAGE_IN_UNION(svc_sounds);
    DECLARE_MESSAGE_IN_UNION(svc_setview);
    DECLARE_MESSAGE_IN_UNION(svc_fixangle);
    DECLARE_MESSAGE_IN_UNION(svc_crosshair_angle);
    DECLARE_MESSAGE_IN_UNION(svc_bsp_decal);
    DECLARE_MESSAGE_IN_UNION(svc_user_message);
    DECLARE_MESSAGE_IN_UNION(svc_entity_message);
    DECLARE_MESSAGE_IN_UNION(svc_game_event);
    DECLARE_MESSAGE_IN_UNION(svc_packet_entities);
    DECLARE_MESSAGE_IN_UNION(svc_splitscreen);
    DECLARE_MESSAGE_IN_UNION(svc_temp_entities);
    DECLARE_MESSAGE_IN_UNION(svc_prefetch);
    DECLARE_MESSAGE_IN_UNION(svc_menu);
    DECLARE_MESSAGE_IN_UNION(svc_game_event_list);
    DECLARE_MESSAGE_IN_UNION(svc_get_cvar_value);
    DECLARE_MESSAGE_IN_UNION(svc_cmd_key_values);
    DECLARE_MESSAGE_IN_UNION(svc_paintmap_data);
  };
};

typedef struct dg_packet_net_message packet_net_message;

enum dg_game { csgo, l4d, l4d2, portal2, orangebox, steampipe };

struct dg_demver_data {
  enum dg_game game : 3;
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
  net_message_type *netmessage_array;
  unsigned int netmessage_count;
  unsigned int network_protocol;
  unsigned int l4d2_version;
};

typedef struct dg_demver_data dg_demver_data;
struct dg_packet;

struct packet_parsed {
  packet_net_message *messages;
  uint32_t message_count;
  dg_bitstream leftover_bits;
  struct dg_packet orig;
};

typedef struct packet_parsed packet_parsed;

#ifdef __cplusplus
}
#endif
