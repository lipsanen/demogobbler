#include "gtest/gtest.h"
extern "C" {
  #include "demogobbler/version_utils.h"
}

void test_l4d2_version(bool expected, int expected_build, const char* str) {
  int build_number = 0;
  bool rval = get_l4d2_build(str, &build_number);
  EXPECT_EQ(rval, expected);
  if(expected && rval) {
    EXPECT_EQ(expected_build, build_number);
  }
}

TEST(L4D2_version, works) {
  test_l4d2_version(true, 4027, "\nLeft 4 Dead 2\nMap: c2m1_highway\nPlayers: 1 (0 bots) / 4 humans\nBuild: 4027\nServer Number: 1\n\n");
  test_l4d2_version(true, 4183, "\nLeft 4 Dead 2\nMap: c6m1_riverbank\nPlayers: 1 (0 bots) / 4 humans\nBuild: 4183\nServer Number: 2\n\n");
  test_l4d2_version(true, 4261, "\nLeft 4 Dead 2\nMap: c3m3_shantytown\nPlayers: 1 (0 bots) / 4 humans\nBuild: 4261\nServer Number: 5\n\n");
  test_l4d2_version(true, 4346, "\nLeft 4 Dead 2\nMap: c2m2_fairgrounds\nPlayers: 4 (0 bots) / 4 humans\nBuild: 4346\nServer Number: 13\n\n");
  test_l4d2_version(true, 4490, "\nLeft 4 Dead 2\nMap: c13m2_southpinestream\nPlayers: 1 (0 bots) / 4 humans\nBuild: 4490\nServer Number: 3\n\n");
  test_l4d2_version(true, 4632, "\nLeft 4 Dead 2\nMap: c11m2_offices\nPlayers: 1 (0 bots) / 4 humans\nBuild: 4632\nServer Number: 3\n\n");
  test_l4d2_version(true, 4710, "\nLeft 4 Dead 2\nMap: c6m2_bedlam\nPlayers: 1 (0 bots) / 4 humans\nBuild: 4710\nServer Number: 3\n\n");
  test_l4d2_version(true, 6403, "\nLeft 4 Dead 2\nMap: c2m2_fairgrounds\nPlayers: 1 (0 bots) / 4 humans\nBuild: 6403\nServer Number: 1\n\n");
  test_l4d2_version(true, 7970, "\nLeft 4 Dead 2\nMap: c5m4_quarter\nPlayers: 1 (0 bots) / 4 humans\nBuild: 7970\nServer Number: 1\n\n");
  test_l4d2_version(true, 8267, "\nLeft 4 Dead 2\nMap: c2m2_fairgrounds\nPlayers: 1 (0 bots) / 4 humans\nBuild: 8267\nServer Number: 4\n\n");
  test_l4d2_version(true, 8491, "\nLeft 4 Dead 2\nMap: c10m1_caves\nPlayers: 1 (0 bots) / 4 humans\nBuild: 8491\nServer Number: 1\n\n");
}

TEST(L4D2_version, random_other_strings) {
  test_l4d2_version(false, 0, "user has paused the game.");
}