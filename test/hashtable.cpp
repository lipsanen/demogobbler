extern "C" {
#include "demogobbler/hashtable.h"
}

#include "gtest/gtest.h"

TEST(dg_hashtable, works) {
  auto table = dg_hashtable_create(100);
  dg_hashtable_entry entry;
  entry.str = "Hello, world!";
  entry.value = 42;
  bool success = dg_hashtable_insert(&table, entry);
  EXPECT_EQ(success, true);

  bool success2 = dg_hashtable_insert(&table, entry);

  EXPECT_EQ(success2, false);

  dg_hashtable_entry got = dg_hashtable_get(&table, "Hello, world!");

  EXPECT_EQ(got.str, entry.str);
  EXPECT_EQ(got.value, entry.value);

  dg_hashtable_entry got2 = dg_hashtable_get(&table, "Nonexistent string");
  EXPECT_EQ(got2.str, nullptr);

  dg_hashtable_free(&table);
}
