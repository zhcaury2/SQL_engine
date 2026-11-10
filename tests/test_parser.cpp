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

static std::string errorFor(const std::string& sql) {
    try {
        Parser p(parseSQLquery(sql));
        p.parseSelect();
    } catch (const std::runtime_error& e) {
        return e.what();
    }
    return "";
}

TEST(ParseErrors, MissingColumnNamesTheProblem) {
    std::string msg = errorFor("SELECT FROM users");
    EXPECT_NE(msg.find("identifier"), std::string::npos);
    EXPECT_NE(msg.find("FROM"), std::string::npos);
}

TEST(ParseErrors, EndOfInputIsNamed) {
    EXPECT_NE(errorFor("SELECT name").find("end of input"), std::string::npos);
}

TEST(ParseErrors, ExpectedTextIsNeverBlank) {
    EXPECT_EQ(errorFor("SELECT FROM users").find("token: ,"), std::string::npos);
}