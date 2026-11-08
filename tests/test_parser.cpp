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
    EXPECT_FALSE(stmt.hasCondition);
}

TEST(Parser, ParseSelectWithStar) {
    auto t = parseSQLquery("SELECT * FROM users");
    Parser parser(t);
    auto stmt = parser.parseSelect();
    ASSERT_EQ(stmt.columns.size(), 1u);
    EXPECT_EQ(stmt.columns[0], "*");
    EXPECT_EQ(stmt.table, "users");
    EXPECT_FALSE(stmt.hasCondition);
}

TEST(Parser, ThrowsOnNoSelect) {
    auto t = parseSQLquery("FROM users");
    Parser parser(t);
    EXPECT_THROW(parser.parseSelect(), std::runtime_error);
}

TEST(Parser, ThrowsOnNoFrom) {
    auto t = parseSQLquery("SELECT name");
    Parser parser(t);
    EXPECT_THROW(parser.parseSelect(), std::runtime_error);
}

TEST(Parser, ThrowsOnNoTable) {
    auto t = parseSQLquery("SELECT name FROM");
    Parser parser(t);
    EXPECT_THROW(parser.parseSelect(), std::runtime_error);
}

TEST(Parser, ThrowsOnNoColumns){
    auto t = parseSQLquery("SELECT FROM users");
    Parser parser(t);
    EXPECT_THROW(parser.parseSelect(), std::runtime_error);
}

TEST(Parser, ThrowsOnInvalidEnd){
    auto t = parseSQLquery("SELECT name FROM users WHERE age>21 something");
    Parser parser(t);
    EXPECT_THROW(parser.parseSelect(), std::runtime_error);
}

TEST(Parser, ThrowsOnTrailingComma){
    auto t = parseSQLquery("SELECT name, FROM users");
    Parser parser(t);
    EXPECT_THROW(parser.parseSelect(), std::runtime_error);
}

TEST(Parser, ThrowsOnInvalidConditionSymbol){
    auto t = parseSQLquery("SELECT name FROM users WHERE age+21");
    Parser parser(t);
    EXPECT_THROW(parser.parseSelect(), std::runtime_error);
}

TEST(Parser, CommaStringIsNotTreatedAsColumnSeparator) {
    auto t = parseSQLquery("SELECT name FROM users WHERE name='John,Doe'");
    Parser parser(t);
    auto stmt = parser.parseSelect();
    EXPECT_EQ(stmt.conditionValue, "John,Doe");
}

TEST(Parser, CommaStringInColumnsThrows){
    auto t = parseSQLquery("SELECT name ',' age FROM users");
    Parser parser(t);
    EXPECT_THROW(parser.parseSelect(), std::runtime_error);
}

TEST(Parser, StarThenColumnThrows){
    auto t = parseSQLquery("SELECT * name FROM users");
    Parser parser(t);
    EXPECT_THROW(parser.parseSelect(), std::runtime_error);
}

TEST(Parser, NegativeOrBadValue){
    auto t = parseSQLquery("SELECT name FROM users WHERE age>name");
    Parser parser(t);
    EXPECT_THROW(parser.parseSelect(), std::runtime_error);
}