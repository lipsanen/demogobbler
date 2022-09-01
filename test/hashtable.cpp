extern "C" {
#include "hashtable.h"
}

#include "gtest/gtest.h"

TEST(hashtable, works) {
  auto table = demogobbler_hashtable_create(100);
  hashtable_entry entry;
  entry.str = "Hello, world!";
  entry.value = 42;
  bool success = demogobbler_hashtable_insert(&table, entry);
  EXPECT_EQ(success, true);

  bool success2 = demogobbler_hashtable_insert(&table, entry);

  EXPECT_EQ(success2, false);

  hashtable_entry got = demogobbler_hashtable_get(&table, "Hello, world!");

  EXPECT_EQ(got.str, entry.str);
  EXPECT_EQ(got.value, entry.value);

  hashtable_entry got2 = demogobbler_hashtable_get(&table, "Nonexistent string");
  EXPECT_EQ(got2.str, nullptr);

  demogobbler_hashtable_free(&table);
}
