#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct dg_packet_net_message;
struct dg_demver_data;
struct dg_bitwriter;

void dg_bitwriter_write_netmessage(struct dg_bitwriter *writer, struct dg_demver_data *version,
                                   struct dg_packet_net_message *message);

#undef DECLARE_MESSAGE_IN_UNION

#ifdef __cplusplus
}
#endif
