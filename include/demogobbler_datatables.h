#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "packettypes.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  sendproptype_int,
  sendproptype_float,
  sendproptype_vector3,
  sendproptype_vector2, // only in new protocol
  sendproptype_string,
  sendproptype_array,
  sendproptype_datatable,
  sendproptype_invalid
} demogobbler_sendproptype;

typedef struct {
  float low_value;
  float high_value;
} prop_values;

// Forward declaration for the array pointer
struct demogobbler_sendprop;
typedef struct demogobbler_sendprop demogobbler_sendprop;

struct demogobbler_sendprop {
  const char *name;
  union {
    prop_values prop_;
    const char* exclude_name;
    const char* dtname; // Datatable or exclude flag is set
    demogobbler_sendprop* array_prop;
  };
  
  unsigned priority : 8;
  unsigned prop_numbits : 7;
  demogobbler_sendproptype proptype : 4;
  unsigned flag_unsigned: 1;
  unsigned flag_coord : 1;
  unsigned flag_noscale : 1;
  unsigned flag_rounddown : 1;
  unsigned flag_roundup : 1;
  unsigned flag_normal : 1;
  unsigned flag_exclude : 1;
  unsigned flag_xyze : 1;
  unsigned flag_insidearray : 1;
  unsigned flag_proxyalwaysyes : 1;
  unsigned flag_changesoften : 1; 
  unsigned flag_isvectorelem : 1;
  unsigned flag_collapsible : 1;
  unsigned flag_coordmp : 1;
  unsigned flag_coordmplp : 1;
  unsigned flag_coordmpint : 1;
  unsigned flag_cellcoord : 1;
  unsigned flag_cellcoordlp : 1;
  unsigned flag_cellcoordint : 1;
  unsigned array_num_elements : 10;

};

typedef struct {

  const char *name;
  demogobbler_sendprop *props;
  size_t prop_count;
  bool needs_decoder;

} demogobbler_sendtable;

typedef struct {
  uint16_t serverclass_id;
  const char *serverclass_name;
  const char *datatable_name;

} demogobbler_serverclass;

typedef struct {
  demogobbler_message_preamble preamble;
  demogobbler_sendtable *sendtables;
  size_t sendtable_count;
  demogobbler_serverclass *serverclasses;
  size_t serverclass_count;

  void* _raw_buffer;
  size_t _raw_buffer_bytes;
} demogobbler_datatables_parsed;

#ifdef __cplusplus
}
#endif
