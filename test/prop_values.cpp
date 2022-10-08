
#include "gtest/gtest.h"
extern "C" {
    #include "parser_entity_state.h"
}

TEST(eproparr, works) {
    eproparr props = demogobbler_eproparr_init(200);
    prop_value_inner* val1 = demogobbler_eproparr_get(&props, 20);
    prop_value_inner* val2 = demogobbler_eproparr_get(&props, 35);

    EXPECT_EQ(val1 - props.values, 20);
    EXPECT_EQ(val2 - props.values, 35);

    prop_value_inner* val1_get = demogobbler_eproparr_next(&props, NULL);
    prop_value_inner* val2_get = demogobbler_eproparr_next(&props, val1_get);
    prop_value_inner* val3_get = demogobbler_eproparr_next(&props, val2_get);

    EXPECT_EQ(val1, val1_get);
    EXPECT_EQ(val2, val2_get);
    EXPECT_EQ(val3_get, nullptr);
    demogobbler_eproparr_free(&props);
}

TEST(eproplist, works) {
    eproplist list = demogobbler_eproplist_init();
    epropnode* node1 = demogobbler_eproplist_get(&list, nullptr, 10);
    epropnode* node2 = demogobbler_eproplist_get(&list, nullptr, 20);
    epropnode* node3 = demogobbler_eproplist_get(&list, nullptr, 5);

    epropnode* node3_get = demogobbler_eproplist_get(&list, nullptr, 5);
    epropnode* node1_get = demogobbler_eproplist_next(&list, node3_get);
    epropnode* node2_get = demogobbler_eproplist_next(&list, node1_get);
    epropnode* null_get = demogobbler_eproplist_next(&list, node2_get);

    EXPECT_EQ(node1_get, node1);
    EXPECT_EQ(node2_get, node2);
    EXPECT_EQ(node3_get, node3);
    EXPECT_EQ(null_get, nullptr);
    demogobbler_eproplist_free(&list);
}

TEST(eproplist, works2) {
    eproplist list = demogobbler_eproplist_init();
    epropnode* node0 = demogobbler_eproplist_get(&list, nullptr, 0);
    epropnode* node1 = demogobbler_eproplist_get(&list, node0, 10);
    epropnode* node2 = demogobbler_eproplist_get(&list, node1, 11);
    epropnode* node1_copy = demogobbler_eproplist_get(&list, nullptr, 10);
    EXPECT_EQ(node1, node1_copy);
    demogobbler_eproplist_free(&list);
}
