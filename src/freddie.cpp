#include "demogobbler/freddie.hpp"
#include <cstring>

using namespace freddie;

demo::demo()
{
    const size_t INITIAL_ARENA_SIZE = 1 << 20;
    arena = dg_arena_create(INITIAL_ARENA_SIZE);
}

demo::~demo()
{
    dg_arena_free(&arena);
}

static void handle_header(parser_state *_state, struct dg_header *header)
{
    demo* state = (demo*)_state->client_state;
    state->header = *header;
}

static void handle_version(parser_state *_state, dg_demver_data data)
{
    demo* state = (demo*)_state->client_state;
    state->demver_data = data;
}

#define HANDLE_PACKET(type) static void handle_ ## type (parser_state* _state, type* packet) { \
    demo* state = (demo*)_state->client_state; \
    auto copy = state->copy_packet(packet); \
    state->packets.push_back(copy); \
}

HANDLE_PACKET(dg_consolecmd);
HANDLE_PACKET(dg_customdata);
HANDLE_PACKET(dg_datatables_parsed);
HANDLE_PACKET(dg_stop);
HANDLE_PACKET(dg_stringtables_parsed);
HANDLE_PACKET(dg_synctick);
HANDLE_PACKET(dg_usercmd);
HANDLE_PACKET(packet_parsed);

dg_parse_result demo::parse_demo(demo* output, void* stream, dg_input_interface interface)
{
    dg_settings settings;
    dg_settings_init(&settings);
    settings.user_arena = &output->arena;
    settings.client_state = output;
    settings.header_handler = handle_header;
    settings.demo_version_handler = handle_version;
    settings.stop_handler = handle_dg_stop;
    settings.consolecmd_handler = handle_dg_consolecmd;
    settings.customdata_handler = handle_dg_customdata;
    settings.datatables_parsed_handler = handle_dg_datatables_parsed;
    settings.stringtables_parsed_handler = handle_dg_stringtables_parsed;
    settings.packet_parsed_handler = handle_packet_parsed;
    settings.stop_handler = handle_dg_stop;
    settings.synctick_handler = handle_dg_synctick;
    settings.usercmd_handler = handle_dg_usercmd;

    return dg_parse(&settings, stream, interface);
}

dg_parse_result demo::write_demo(void* stream, dg_output_interface interface)
{
    dg_parse_result result;
    std::memset(&result, 0, sizeof(result));
    dg_writer writer;
    dg_writer_init(&writer);
    writer.version = demver_data;
    dg_writer_open(&writer, stream, interface);
    dg_write_header(&writer, &header);

    for(size_t i=0; i < packets.size(); ++i)
    {
        demo_packet packet = packets[i];
        if(std::holds_alternative<packet_parsed*>(packet))
        {
            packet_parsed* ptr = std::get<packet_parsed*>(packet);
            dg_write_packet_parsed(&writer, ptr);
        }
        else if(std::holds_alternative<dg_datatables_parsed*>(packet))
        {
            dg_datatables_parsed* ptr = std::get<dg_datatables_parsed*>(packet);
            dg_write_datatables_parsed(&writer, ptr);
        }
        else if(std::holds_alternative<dg_stringtables_parsed*>(packet))
        {
            dg_stringtables_parsed* ptr = std::get<dg_stringtables_parsed*>(packet);
            dg_write_stringtables_parsed(&writer, ptr);
        }
        else if(std::holds_alternative<dg_consolecmd*>(packet))
        {
            dg_consolecmd* ptr = std::get<dg_consolecmd*>(packet);
            dg_write_consolecmd(&writer, ptr);
        }
        else if(std::holds_alternative<dg_usercmd*>(packet))
        {
            dg_usercmd* ptr = std::get<dg_usercmd*>(packet);
            dg_write_usercmd(&writer, ptr);
        }
        else if(std::holds_alternative<dg_stop*>(packet))
        {
            dg_stop* ptr = std::get<dg_stop*>(packet);
            dg_write_stop(&writer, ptr);
        }
        else if(std::holds_alternative<dg_synctick*>(packet))
        {
            dg_synctick* ptr = std::get<dg_synctick*>(packet);
            dg_write_synctick(&writer, ptr);
        }
        else if(std::holds_alternative<dg_customdata*>(packet))
        {
            dg_customdata* ptr = std::get<dg_customdata*>(packet);
            dg_write_customdata(&writer, ptr);
        }
        else
        {
            result.error = true;
            result.error_message = "unknown demo packet";
            break;
        }
    }

    dg_writer_close(&writer);

    return result;
}
