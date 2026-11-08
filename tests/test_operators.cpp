#include <gtest/gtest.h>
#include "operators.h"

static Table makeTable(std::vector<Row> rows) {
    Table t;
    t.name = "t";
    t.columns = {"a", "b", "c"};
    t.rows = std::move(rows);
    return t;
}

TEST(ScanTest, NextRow) {
    Table table;
    table.columns = {"a", "b", "c"};
    table.rows = {
        {1.0, 2.0, 3.0},
        {4.0, 5.0, 6.0}
    };

    Scan scan(table);
    Row out;

    ASSERT_TRUE(scan.next(out));
    EXPECT_EQ(out, (Row{1.0, 2.0, 3.0}));

    ASSERT_TRUE(scan.next(out));
    EXPECT_EQ(out, (Row{4.0, 5.0, 6.0}));

    EXPECT_FALSE(scan.next(out));
}

TEST(ScanTest, EmptyTable) {
    Table table;
    Scan scan(table);
    Row out;

    EXPECT_FALSE(scan.next(out));
}

TEST(FilterTest, NextRow) {
    Table table;
    table.columns = {"a", "b", "c"};
    table.rows = {
        {1.0, 2.0, 3.0},
        {4.0, 5.0, 6.0}
    };

    Scan scan(table);
    Filter filter(table, scan);
    Row out;

    ASSERT_TRUE(filter.next(out));
    EXPECT_EQ(out, (Row{1.0, 2.0, 3.0}));

    ASSERT_TRUE(filter.next(out));
    EXPECT_EQ(out, (Row{4.0, 5.0, 6.0}));

    EXPECT_FALSE(filter.next(out));
}

TEST(FilterTest, EmptyTable) {
    Table table;
    Scan scan(table);
    Filter filter(table, scan);
    Row out;

    EXPECT_FALSE(filter.next(out));
}

TEST(FilterTest, RecursiveFiltering){
    Table table = makeTable({
        {1.0, 2.0, 3.0},
        {4.0, 5.0, 6.0}
    });

    Scan scan(table);
    Filter filter1(table, scan);
    Filter filter2(table, filter1);
    Row out;

    ASSERT_TRUE(filter2.next(out));
    EXPECT_EQ(out, (Row{1.0, 2.0, 3.0}));

    ASSERT_TRUE(filter2.next(out));
    EXPECT_EQ(out, (Row{4.0, 5.0, 6.0}));

    EXPECT_FALSE(filter2.next(out));
}