
#include "gtest/gtest.h"
extern "C" {
#include "parser_entity_state.h"
}

TEST(dg_eproparr, works) {
  bool newprop;
  dg_eproparr props = dg_eproparr_init(200);
  dg_prop_value_inner *val1 = dg_eproparr_get(&props, 20, &newprop);
  dg_prop_value_inner *val2 = dg_eproparr_get(&props, 35, &newprop);

  EXPECT_EQ(val1 - props.values, 20);
  EXPECT_EQ(val2 - props.values, 35);

  dg_prop_value_inner *val1_get = dg_eproparr_next(&props, NULL);
  dg_prop_value_inner *val2_get = dg_eproparr_next(&props, val1_get);
  dg_prop_value_inner *val3_get = dg_eproparr_next(&props, val2_get);

  EXPECT_EQ(val1, val1_get);
  EXPECT_EQ(val2, val2_get);
  EXPECT_EQ(val3_get, nullptr);
  dg_eproparr_free(&props);
}

TEST(dg_eproplist, works) {
  bool newprop;
  dg_eproplist list = dg_eproplist_init();
  dg_epropnode *node1 = dg_eproplist_get(&list, nullptr, 10, &newprop);
  dg_epropnode *node2 = dg_eproplist_get(&list, nullptr, 20, &newprop);
  dg_epropnode *node3 = dg_eproplist_get(&list, nullptr, 5, &newprop);

  dg_epropnode *node3_get = dg_eproplist_get(&list, nullptr, 5, &newprop);
  dg_epropnode *node1_get = dg_eproplist_next(&list, node3_get);
  dg_epropnode *node2_get = dg_eproplist_next(&list, node1_get);
  dg_epropnode *null_get = dg_eproplist_next(&list, node2_get);

  EXPECT_EQ(node1_get, node1);
  EXPECT_EQ(node2_get, node2);
  EXPECT_EQ(node3_get, node3);
  EXPECT_EQ(null_get, nullptr);
  dg_eproplist_free(&list);
}

TEST(dg_eproplist, works2) {
  bool newprop;
  dg_eproplist list = dg_eproplist_init();
  dg_epropnode *node0 = dg_eproplist_get(&list, nullptr, 0, &newprop);
  dg_epropnode *node1 = dg_eproplist_get(&list, node0, 10, &newprop);
  dg_eproplist_get(&list, node1, 11, &newprop);
  dg_epropnode *node1_copy = dg_eproplist_get(&list, nullptr, 10, &newprop);
  EXPECT_EQ(node1, node1_copy);
  dg_eproplist_free(&list);
}
