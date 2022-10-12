#pragma once

#include "demogobbler/packet_netmessages.h"
#include "parser.h"

void dg_parse_packetentities(parser *thisptr, struct dg_svc_packet_entities *message);
