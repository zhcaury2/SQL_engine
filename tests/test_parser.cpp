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

static SelectStatement parseOnly(const std::string& sql) {
    Parser p(parseSQLquery(sql));
    return p.parseSelect();
}

static std::string showOrder(const SelectStatement& s) {
    std::string out;
    for (const auto& k : s.orderBy) {
        if (!out.empty()) out += ",";
        out += k.column + (k.descending ? " DESC" : " ASC");
    }
    return out;
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

TEST(Parser, NeitherClausePresent) {
    auto s = parseOnly("SELECT * FROM t");
    EXPECT_TRUE(s.orderBy.empty());
    EXPECT_FALSE(s.limit.has_value());
}

TEST(Parser, OrderByDefaultsToAscending) {
    EXPECT_EQ(showOrder(parseOnly("SELECT * FROM t ORDER BY a")), "a ASC");
}

TEST(Parser, ExplicitAscAndDesc) {
    EXPECT_EQ(showOrder(parseOnly("SELECT * FROM t ORDER BY a ASC")), "a ASC");
    EXPECT_EQ(showOrder(parseOnly("SELECT * FROM t ORDER BY a DESC")), "a DESC");
}

TEST(Parser, MultipleKeysKeepTheirOrderAndDirections) {
    EXPECT_EQ(showOrder(parseOnly("SELECT * FROM t ORDER BY a DESC, b, c ASC")),
              "a DESC,b ASC,c ASC");
}

TEST(Parser, KeywordsAnyCaseAndColumnsLowercased) {
    EXPECT_EQ(showOrder(parseOnly("select * from t order by Year desc")), "year DESC");
}

TEST(Parser, LimitValue) {
    auto s = parseOnly("SELECT * FROM t LIMIT 5");
    ASSERT_TRUE(s.limit.has_value());
    EXPECT_EQ(*s.limit, 5u);
}

TEST(Parser, LimitZeroIsAllowed) {
    auto s = parseOnly("SELECT * FROM t LIMIT 0");
    ASSERT_TRUE(s.limit.has_value());
    EXPECT_EQ(*s.limit, 0u);
}

TEST(Parser, OrderByAndLimitTogether) {
    auto s = parseOnly("SELECT * FROM t ORDER BY a DESC LIMIT 3");
    EXPECT_EQ(showOrder(s), "a DESC");
    ASSERT_TRUE(s.limit.has_value());
    EXPECT_EQ(*s.limit, 3u);
}

TEST(Parser, AfterCompoundWhere) {
    auto s = parseOnly("SELECT a FROM t WHERE a = 1 OR (b = 2 AND c = 3) ORDER BY a LIMIT 10");
    ASSERT_TRUE(s.where != nullptr);
    EXPECT_EQ(s.where->kind, Expr::Kind::Or);
    EXPECT_EQ(showOrder(s), "a ASC");
    EXPECT_EQ(*s.limit, 10u);
}

TEST(Parser, TrailingSemicolon) {
    auto s = parseOnly("SELECT * FROM t ORDER BY a LIMIT 3;");
    EXPECT_EQ(showOrder(s), "a ASC");
    EXPECT_EQ(*s.limit, 3u);
}

TEST(Parser, ColumnNamesThatMerelyStartWithKeywordsAreFine) {
    auto s = parseOnly("SELECT orders FROM t ORDER BY orders, limiter");
    EXPECT_EQ(showOrder(s), "orders ASC,limiter ASC");
}

TEST(Parser, SelectListAndWhereAreUnaffected) {
    auto s = parseOnly("SELECT a, b FROM t WHERE a > 1 ORDER BY b");
    EXPECT_EQ(s.columns.size(), 2u);
    EXPECT_EQ(s.table, "t");
    EXPECT_TRUE(s.where != nullptr);
}

TEST(Parser, MalformedClausesThrow) {
    for (const char* sql : {
        "SELECT * FROM t ORDER a",                    // missing BY
        "SELECT * FROM t ORDER BY",                   // no column
        "SELECT * FROM t ORDER BY ,a",                // leading comma
        "SELECT * FROM t ORDER BY a,",                // trailing comma
        "SELECT * FROM t ORDER BY a b",               // missing comma
        "SELECT * FROM t ORDER BY a DESC DESC",       // doubled direction
        "SELECT * FROM t ORDER BY a DESC,",           // trailing comma after direction
        "SELECT * FROM t ORDER BY 1",                 // number, not a column
        "SELECT * FROM t ORDER BY 'a'",               // string, not a column
        "SELECT * FROM t ORDER BY a ORDER BY b",      // repeated clause
        "SELECT * FROM t ORDER BY a WHERE a = 1",     // WHERE after ORDER BY
        "SELECT * FROM t WHERE ORDER BY a",           // WHERE with no condition
        "SELECT * FROM t LIMIT",                      // no count
        "SELECT * FROM t LIMIT -1",                   // negative
        "SELECT * FROM t LIMIT 2.5",                  // not an integer
        "SELECT * FROM t LIMIT 'x'",                  // string
        "SELECT * FROM t LIMIT a",                    // identifier
        "SELECT * FROM t LIMIT 1e3",                  // scientific notation
        "SELECT * FROM t LIMIT 5 10",                 // extra number
        "SELECT * FROM t LIMIT 5 LIMIT 6",            // repeated clause
        "SELECT * FROM t LIMIT 5 ORDER BY a",         // wrong clause order
        "SELECT * FROM t ORDER BY a LIMIT",           // LIMIT with no count
        "SELECT * FROM t LIMIT 99999999999999999999999",  // out of range
    }) {
        Parser p(parseSQLquery(sql));
        EXPECT_THROW(p.parseSelect(), std::runtime_error) << sql;
    }
}