#pragma once

#include "demogobbler/packet_netmessages.h"
#include "demogobbler/parser.h"

void dg_parser_handle_packetentities(dg_parser *thisptr, struct dg_svc_packet_entities *message);
