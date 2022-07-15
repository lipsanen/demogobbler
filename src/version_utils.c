#include "version_utils.h"
#include "utils.h"
#include <string.h>

struct version_pair
{
  const char* game_directory;
  demo_version version;
};

typedef struct version_pair version_pair;

static version_pair versions[] = {
  {"portal2", portal2},
  {"csgo", csgo},
  {"left4dead2", l4d2},
  {"left4dead", l4d}
};

demo_version get_demo_version(demogobbler_header* header)
{
  for(size_t i=0; i < ARRAYSIZE(versions); ++i)
  {
    if(strcmp(header->game_directory, versions[i].game_directory) == 0)
    {
      return versions[i].version;
    }
  }

  return orangebox;
}
