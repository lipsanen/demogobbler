#include "freddie.h"
#include <stdio.h>

int main(int argc, char **argv) {
  if (argc <= 1) {
    printf("Usage: mercury <filepath>\n");
    return 0;
  }

  demogobbler_freddie output = demogobbler_freddie_file(argv[1]);
  demogobbler_header* header = &output.header;

  printf("ID: %s\n", header->ID);
  printf("Demo protocol: %d\n", header->demo_protocol);
  printf("Net protocol: %d\n", header->net_protocol);
  printf("Server name: %s\n", header->server_name);
  printf("Client name: %s\n", header->client_name);
  printf("Map name: %s\n", header->map_name);
  printf("Game directory: %s\n", header->game_directory);
  printf("Seconds: %f\n", header->seconds);
  printf("Tick count: %d\n", header->tick_count);
  printf("Frame count: %d\n", header->frame_count);
  printf("Signon length: %d\n", header->signon_length);

  printf("Got %d cmd infos\n", output.cmdinfo_count);

  for(size_t i=0; i < output.consolecmd_count; ++i) {
    freddie_consolecmd cmd = output.consolecmd[i];
    if(cmd.tick >= 0)
      printf("%d: %s\n", cmd.tick, cmd.cmd);
  }

  demogobbler_freddie_free(&output);
}
