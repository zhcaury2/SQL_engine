#include "parse.h"
#include "tokenize.h"
#include <gtest/gtest.h>

TEST(Parser, ParseSelect) {
    auto t = parseSQLquery("SELECT name FROM users WHERE age>21");
    Parser parser(t);
    auto stmt = parser.parseSelect();
    ASSERT_EQ(stmt.columns.size(), 1u);
    EXPECT_EQ(stmt.columns[0], "name");
    EXPECT_EQ(stmt.table, "users");
}

TEST(Parser, ParseSelectWithCondition) {
    auto t = parseSQLquery("SELECT name FROM users WHERE age>21");
    Parser parser(t);
    auto stmt = parser.parseSelect();
    EXPECT_TRUE(stmt.hasCondition);
    EXPECT_EQ(stmt.conditionColumn, "age");
    EXPECT_EQ(stmt.conditionSymbol, ">");
    EXPECT_EQ(stmt.conditionValue, "21");
}

TEST(Parser, ParseSelectWithMultipleColumns) {
    auto t = parseSQLquery("SELECT name, age FROM users");
    Parser parser(t);
    auto stmt = parser.parseSelect();
    ASSERT_EQ(stmt.columns.size(), 2u);
    EXPECT_EQ(stmt.columns[0], "name");
    EXPECT_EQ(stmt.columns[1], "age");
    EXPECT_EQ(stmt.table, "users");
}