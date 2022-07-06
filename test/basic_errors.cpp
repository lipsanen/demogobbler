#include "gtest/gtest.h"
#include "demogoblin.h"

TEST(BasicErrors, NullPointerReturnsError)
{
    demogoblin_settings settings;
    EXPECT_EQ(demogoblin_parse(NULL, settings), false);
}
