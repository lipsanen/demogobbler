
#include "gtest/gtest.h"
extern "C" {
    #include "parser_entity_state.h"
}

TEST(eproparr, works) {
    bool newprop;
    eproparr props = demogobbler_eproparr_init(200);
    prop_value_inner* val1 = demogobbler_eproparr_get(&props, 20, &newprop);
    prop_value_inner* val2 = demogobbler_eproparr_get(&props, 35, &newprop);

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
    bool newprop;
    eproplist list = demogobbler_eproplist_init();
    epropnode* node1 = demogobbler_eproplist_get(&list, nullptr, 10, &newprop);
    epropnode* node2 = demogobbler_eproplist_get(&list, nullptr, 20, &newprop);
    epropnode* node3 = demogobbler_eproplist_get(&list, nullptr, 5, &newprop);

    epropnode* node3_get = demogobbler_eproplist_get(&list, nullptr, 5, &newprop);
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
    bool newprop;
    eproplist list = demogobbler_eproplist_init();
    epropnode* node0 = demogobbler_eproplist_get(&list, nullptr, 0, &newprop);
    epropnode* node1 = demogobbler_eproplist_get(&list, node0, 10, &newprop);
    demogobbler_eproplist_get(&list, node1, 11, &newprop);
    epropnode* node1_copy = demogobbler_eproplist_get(&list, nullptr, 10, &newprop);
    EXPECT_EQ(node1, node1_copy);
    demogobbler_eproplist_free(&list);
}
