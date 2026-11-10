#include <gtest/gtest.h>
#include "operators.h"

#include <gtest/gtest.h>
#include <variant>
#include "operators.h"

static Table makeUsers() {
    Table t;
    t.name = "users";
    t.columns = {"username", "password", "age"};
    for (size_t i = 0; i < t.columns.size(); ++i) t.colIndex[t.columns[i]] = i;
    t.rows = {
        {std::string("a"), std::string("a2"), 18.0},
        {std::string("b"), std::string("b2"), 19.0},
        {std::string("c"), std::string("c2"), 20.0},
    };
    return t;
}

using Names = std::vector<std::string>;

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
TEST(Project, SelectsSingleColumn) {
    Table t = makeUsers();
    Project p(std::make_unique<Scan>(t), {0});
    Row row;
    ASSERT_TRUE(p.next(row));
    ASSERT_EQ(row.size(), 1u);
    EXPECT_EQ(std::get<std::string>(row[0]), "a");

    ASSERT_TRUE(p.next(row));
    EXPECT_EQ(std::get<std::string>(row[0]), "b");

    ASSERT_TRUE(p.next(row));
    EXPECT_EQ(std::get<std::string>(row[0]), "c");

    EXPECT_FALSE(p.next(row));
}

TEST (Project, ReordersColumns) {
    Table t = makeUsers();
    Project p(std::make_unique<Scan>(t), {2, 0});
    Row row;
    ASSERT_TRUE(p.next(row));
    ASSERT_EQ(row.size(), 2u);
    EXPECT_DOUBLE_EQ(std::get<double>(row[0]), 18.0);
    EXPECT_EQ(std::get<std::string>(row[1]), "a");
}

TEST(Project, DuplicateColumns){
    Table t = makeUsers();
    Project p(std::make_unique<Scan>(t), {0, 0});
    Row row;
    ASSERT_TRUE(p.next(row));
    ASSERT_EQ(row.size(), 2u);
    EXPECT_EQ(std::get<std::string>(row[0]), "a");
    EXPECT_EQ(std::get<std::string>(row[1]), "a");
}

TEST(Project, AllColumns){
    Table t = makeUsers();
    Project p(std::make_unique<Scan>(t), {0, 1, 2});
    Row row;
    ASSERT_TRUE(p.next(row));
    ASSERT_EQ(row.size(), 3u);
    EXPECT_EQ(std::get<std::string>(row[0]), "a");
    EXPECT_DOUBLE_EQ(std::get<double>(row[2]), 18.0);

    ASSERT_TRUE(p.next(row));
    EXPECT_EQ(std::get<std::string>(row[0]), "b");
    EXPECT_DOUBLE_EQ(std::get<double>(row[2]), 19.0);

    ASSERT_TRUE(p.next(row));
    EXPECT_EQ(std::get<std::string>(row[0]), "c");
    EXPECT_DOUBLE_EQ(std::get<double>(row[2]), 20.0);

    EXPECT_FALSE(p.next(row));
}

