#include "test_demos.hpp"
#include <filesystem>

std::vector<std::string> get_test_demos()
{
  std::vector<std::string> demos;

  for(auto& file : std::filesystem::directory_iterator("./test_demos/"))
  {
    if(file.is_regular_file() && file.path().extension() == ".dem")
    {
      demos.push_back(file.path().string());
    }
  }

  return demos;
}
