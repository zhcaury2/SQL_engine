#include "parse.h"
#include "tokenize.h"
#include <gtest/gtest.h>

static std::string show(const Expr& e) {
    if (e.kind == Expr::Kind::Comparison)
        return e.column + e.op + e.value;
    std::string op = (e.kind == Expr::Kind::And) ? " AND " : " OR ";
    return "(" + show(*e.left) + op + show(*e.right) + ")";
}

static std::string parsedWhere(const std::string& sql) {
    Parser p(parseSQLquery(sql));
    SelectStatement stmt = p.parseSelect();
    return stmt.where ? show(*stmt.where) : "";
}

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

TEST(Parser, NoWhereGivesNullTree) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t"), "");
}

TEST(Parser, SingleComparisonIsALeaf) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE a = 1"), "a=1");
}

TEST(Parser, SimpleAnd) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE a = 1 AND b = 2"), "(a=1 AND b=2)");
}

TEST(Parser, SimpleOr) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE a = 1 OR b = 2"), "(a=1 OR b=2)");
}

TEST(Parser, AndBindsTighterThanOr) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE a = 1 OR b = 2 AND c = 3"),
              "(a=1 OR (b=2 AND c=3))");
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE a = 1 AND b = 2 OR c = 3"),
              "((a=1 AND b=2) OR c=3)");
}

TEST(Parser, MixedChainGroupsEachAndRun) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE a = 1 AND b = 2 OR c = 3 AND d = 4"),
              "((a=1 AND b=2) OR (c=3 AND d=4))");
}

TEST(Parser, ChainsAreLeftAssociative) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE a = 1 AND b = 2 AND c = 3"),
              "((a=1 AND b=2) AND c=3)");
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE a = 1 OR b = 2 OR c = 3"),
              "((a=1 OR b=2) OR c=3)");
}

TEST(Parser, ParenthesesOverridePrecedence) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE (a = 1 OR b = 2) AND c = 3"),
              "((a=1 OR b=2) AND c=3)");
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE a = 1 AND (b = 2 OR c = 3)"),
              "(a=1 AND (b=2 OR c=3))");
}

TEST(Parser, ParenthesesCanRegroupAChain) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE a = 1 OR (b = 2 OR c = 3)"),
              "(a=1 OR (b=2 OR c=3))");
}

TEST(Parser, RedundantParenthesesLeaveNoTrace) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE ((a = 1))"), "a=1");
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE ((((a = 1))))"), "a=1");
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE (a = 1) AND (b = 2)"), "(a=1 AND b=2)");
}

TEST(Parser, KeywordsAnyCase) {
    EXPECT_EQ(parsedWhere("select * from t where a = 1 and b = 2 or c = 3"),
              "((a=1 AND b=2) OR c=3)");
}

TEST(Parser, StringAndDecimalLeaves) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE n = 'x' AND p <= 9.99"),
              "(n=x AND p<=9.99)");
}

TEST(Parser, AndOrInsideStringIsPartOfTheValue) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE n = 'a AND b'"), "n=a AND b");
}

TEST(Parser, TrailingSemicolonAfterCompoundCondition) {
    EXPECT_EQ(parsedWhere("SELECT * FROM t WHERE a = 1 OR b = 2;"), "(a=1 OR b=2)");
}

TEST(Parser, LeafFieldsAreFilled) {
    Parser p(parseSQLquery("SELECT * FROM t WHERE age >= 18 AND name = 'x'"));
    auto s = p.parseSelect();
    ASSERT_TRUE(s.where != nullptr);
    ASSERT_EQ(s.where->kind, Expr::Kind::And);
    EXPECT_EQ(s.where->left->column, "age");
    EXPECT_EQ(s.where->left->op, ">=");
    EXPECT_EQ(s.where->left->valueType, TokenType::Number);
    EXPECT_EQ(s.where->right->valueType, TokenType::String);
}

TEST(Parser, MalformedConditionsThrow) {
    for (const char* sql : {
        "SELECT * FROM t WHERE",                         // nothing after WHERE
        "SELECT * FROM t WHERE (a = 1",                  // missing )
        "SELECT * FROM t WHERE ((a = 1)",                // still missing one )
        "SELECT * FROM t WHERE a = 1)",                  // extra )
        "SELECT * FROM t WHERE a = 1 AND",               // dangling AND
        "SELECT * FROM t WHERE a = 1 OR",                // dangling OR
        "SELECT * FROM t WHERE AND a = 1",               // leading AND
        "SELECT * FROM t WHERE OR a = 1",                // leading OR
        "SELECT * FROM t WHERE a = 1 AND AND b = 2",     // doubled AND
        "SELECT * FROM t WHERE a = 1 OR OR b = 2",       // doubled OR
        "SELECT * FROM t WHERE a = 1 AND OR b = 2",      // AND followed by OR
        "SELECT * FROM t WHERE ()",                      // empty parentheses
        "SELECT * FROM t WHERE (a = 1 AND)",             // dangling AND inside parens
        "SELECT * FROM t WHERE a = 1 b = 2",             // missing connector
        "SELECT * FROM t WHERE a = 1 AND (b = 2",        // unclosed after AND
        "SELECT * FROM t WHERE a = 1 AND ;",             // operator then terminator
        "SELECT * FROM t WHERE NOT a = 1",               // NOT isn't supported
        "SELECT * FROM t WHERE a = 1 AND (b = 2 OR)",    // dangling OR inside parens
    }) {
        Parser p(parseSQLquery(sql));
        EXPECT_THROW(p.parseSelect(), std::runtime_error) << sql;
    }
}