#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

struct demogobbler_packet_net_message;
struct demo_version_data;
struct demogobbler_bitwriter;

void demogobbler_bitwriter_write_netmessage(struct demogobbler_bitwriter *writer, struct demo_version_data* version, struct demogobbler_packet_net_message* message);

#undef DECLARE_MESSAGE_IN_UNION

#ifdef __cplusplus
}
#endif
