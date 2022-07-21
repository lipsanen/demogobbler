#pragma once

#include "packet_netmessages.h"
#include "parser.h"

void parse_netmessages(parser* thisptr, void* data, size_t size);
