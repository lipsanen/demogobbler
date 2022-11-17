#pragma once

#include "demogobbler/bitstream.h"
#include "demogobbler/vector.h"
#include <stdint.h>

enum dg_usercmd_buttons {
  None = 0,
  Attack = 1,
  Jump = 1 << 1,
  Duck = 1 << 2,
  Forward = 1 << 3,
  Back = 1 << 4,
  Use = 1 << 5,
  Cancel = 1 << 6,
  Left = 1 << 7,
  Right = 1 << 8,
  MoveLeft = 1 << 9,
  MoveRight = 1 << 10,
  Attack2 = 1 << 11,
  Run = 1 << 12,
  Reload = 1 << 13,
  Alt1 = 1 << 14,
  Alt2 = 1 << 15,
  Score = 1 << 16,
  Speed = 1 << 17,
  Walk = 1 << 18,
  Zoom = 1 << 19,
  Weapon1 = 1 << 20,
  Weapon2 = 1 << 21,
  BullRush = 1 << 22,
  Grenade1 = 1 << 23,
  Grenade2 = 1 << 24,
  LookSpin = 1 << 25,
  CurrentAbility = 1 << 26,
  PreviousAbility = 1 << 27,
  Ability1 = 1 << 28,
  Ability2 = 1 << 29,
  Ability3 = 1 << 30,
  Ability4 = 1 << 31
};

typedef enum dg_usercmd_buttons dg_usercmd_buttons;

struct dg_usercmd_vector {
  float x, y, z;
  bool x_exists, y_exists, z_exists;
};

typedef struct dg_usercmd_vector dg_usercmd_vector;

struct dg_usercmd_move {
  float side, forward, up;
  bool side_exists, forward_exists, up_exists;
};

typedef struct dg_usercmd_move dg_usercmd_move;

struct dg_usercmd_parsed {
  const dg_usercmd* orig;
  dg_bitstream left_overbits;
  uint32_t command_number;
  uint32_t tick_count;
  dg_usercmd_vector viewangle;
  dg_usercmd_move movement;
  int32_t buttons;
  uint32_t weapon_select;
  uint32_t weapon_subtype;
  uint16_t mouse_dx;
  uint16_t mouse_dy;
  uint8_t impulse;

  uint8_t command_number_exists : 1;
  uint8_t tick_count_exists : 1;
  uint8_t buttons_exists : 1;
  uint8_t weapon_select_exists : 1;
  uint8_t weapon_subtype_exists : 1;
  uint8_t mouse_dx_exists : 1;
  uint8_t mouse_dy_exists : 1;
  uint8_t impulse_exists : 1;
};

typedef struct dg_usercmd_parsed dg_usercmd_parsed;
