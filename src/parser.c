#include "demogobbler.h"
#include "parser.h"
#include "stddef.h"


void parser_init(parser* thisptr, demogobbler_settings* settings)
{
    thisptr->m_settings = *settings;
}

void parser_parse(parser* thisptr, const char* filepath)
{
    if(!filepath)
        return;

    FILE* stream = fopen(filepath, "rb");

    if(stream)
    {
        filereader_init(&thisptr->m_reader, stream);
        _parse_header(thisptr);
        _parser_mainloop(thisptr);
        filereader_free(&thisptr->m_reader);
    }
}

void _parse_header(parser* thisptr)
{
    demogobbler_header header;
    filereader_readdata(&thisptr->m_reader, header.ID, 8);
    header.demo_protocol = filereader_readint32(&thisptr->m_reader);
    header.net_protocol = filereader_readint32(&thisptr->m_reader);

    if(thisptr->m_settings.header_handler)
    {
        filereader_readdata(&thisptr->m_reader, header.server_name, 260);
        filereader_readdata(&thisptr->m_reader, header.client_name, 260);
        filereader_readdata(&thisptr->m_reader, header.map_name, 260);
        filereader_readdata(&thisptr->m_reader, header.game_directory, 260);
        header.seconds = filereader_readfloat(&thisptr->m_reader);
        header.tick_count = filereader_readint32(&thisptr->m_reader);
        header.event_count = filereader_readint32(&thisptr->m_reader);
        header.signon_length = filereader_readint32(&thisptr->m_reader);

        thisptr->m_settings.header_handler(&header);
    }

    filereader_skipto(&thisptr->m_reader, 0x430);
}

void _parser_mainloop(parser* thisptr)
{

}
