#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler/packettypes.h"
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
} dg_sendproptype;

typedef struct {
  float low_value;
  float high_value;
} dg_prop_values;

// Forward declaration for the array pointer
struct dg_sendprop;
typedef struct dg_sendprop dg_sendprop;
struct dg_sendtable;
typedef struct dg_sendtable dg_sendtable;

struct dg_sendprop {
  const char *name;
  union {
    dg_prop_values prop_;
    const char *exclude_name;
    const char *dtname; // Datatable or exclude flag is set
    dg_sendprop *array_prop;
  };

  union {
    struct dg_sendtable *baseclass; // For sendproptype_datatable this is a pointer to the baseclass
    struct dg_sendtable *owner_class; // For flattened props that are from a derived class this
                                      // points to the class (sendtable) where they came from
  };

  unsigned priority : 8;
  unsigned prop_numbits : 7;
  dg_sendproptype proptype : 4;
  unsigned flag_unsigned : 1;
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

struct dg_sendtable {
  const char *name;
  dg_sendprop *props;
  size_t prop_count;
  bool needs_decoder;
};

struct dg_serverclass {
  uint16_t serverclass_id;
  const char *serverclass_name;
  const char *datatable_name;
};

typedef struct dg_serverclass dg_serverclass;

typedef struct {
  dg_message_preamble preamble;
  dg_sendtable *sendtables;
  size_t sendtable_count;
  dg_serverclass *serverclasses;
  size_t serverclass_count;

  void *_raw_buffer;
  size_t _raw_buffer_bytes;
} dg_datatables_parsed;

typedef struct {
  dg_datatables_parsed output;
  const char *error_message;
  bool error;
} dg_datatables_parsed_rval;

#ifdef __cplusplus
}
#endif
