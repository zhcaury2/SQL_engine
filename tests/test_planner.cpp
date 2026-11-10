#include <gtest/gtest.h>
#include <map>
#include <variant>
#include "planner.h"
#include "parse.h"
#include "tokenize.h"

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

static std::map<std::string, Table> makeCatalog() {
    std::map<std::string, Table> catalog;
    catalog.emplace("users", makeUsers());
    return catalog;
}

static std::vector<Row> runQuery(const std::string& sql,
                                 const std::map<std::string, Table>& catalog) {
    Parser parser(parseSQLquery(sql));
    SelectStatement stmt = parser.parseSelect();
    auto plan = buildPlan(stmt, catalog);
    std::vector<Row> result;
    Row row;
    while (plan->next(row)) result.push_back(row);
    return result;
}

static std::vector<std::string> textColumn(const std::vector<Row>& rows, size_t col) {
    std::vector<std::string> out;
    for (const auto& r : rows) out.push_back(std::get<std::string>(r[col]));
    return out;
}

static std::map<std::string, Table> makeCapitalizedCatalog() {
    Table t;
    t.name = "people";
    t.columns = {"Username", "Age"};
    t.colIndex = {{"username", 0}, {"age", 1}};
    t.rows = {{std::string("a"), 18.0}, {std::string("b"), 19.0}};
    std::map<std::string, Table> c;
    c.emplace("people", std::move(t));
    return c;
}

using Names = std::vector<std::string>;

TEST(Planner, SelectStarReturnsAllRowsAndColumns) {
    auto catalog = makeCatalog();
    auto rows = runQuery("SELECT * FROM users", catalog);
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[0].size(), 3u);
    EXPECT_EQ(std::get<std::string>(rows[0][0]), "a");
    EXPECT_DOUBLE_EQ(std::get<double>(rows[2][2]), 20.0);
}

TEST(Planner, SelectOneColumn) {
    auto catalog = makeCatalog();
    auto rows = runQuery("SELECT username FROM users", catalog);
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[0].size(), 1u);
    EXPECT_EQ(textColumn(rows, 0), (Names{"a", "b", "c"}));
}

TEST(Planner, SelectReordersColumns) {
    auto catalog = makeCatalog();
    auto rows = runQuery("SELECT age, username FROM users", catalog);
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_DOUBLE_EQ(std::get<double>(rows[0][0]), 18.0);
    EXPECT_EQ(std::get<std::string>(rows[0][1]), "a");
}

TEST(Planner, SelectDuplicateColumns) {
    auto catalog = makeCatalog();
    auto rows = runQuery("SELECT username, username FROM users", catalog);
    ASSERT_EQ(rows[0].size(), 2u);
    EXPECT_EQ(std::get<std::string>(rows[0][0]), std::get<std::string>(rows[0][1]));
}

TEST(Planner, KeywordsAreCaseInsensitive) {
    auto catalog = makeCatalog();
    auto rows = runQuery("select username from users where age > 18", catalog);
    EXPECT_EQ(textColumn(rows, 0), (Names{"b", "c"}));
}

TEST(Planner, WhereNumberGreaterThan) {
    auto catalog = makeCatalog();
    auto rows = runQuery("SELECT username FROM users WHERE age > 18", catalog);
    EXPECT_EQ(textColumn(rows, 0), (Names{"b", "c"}));
}

TEST(Planner, WhereNumberEquals) {
    auto catalog = makeCatalog();
    auto rows = runQuery("SELECT username FROM users WHERE age = 19", catalog);
    EXPECT_EQ(textColumn(rows, 0), (Names{"b"}));
}

TEST(Planner, WhereStringEquals) {
    auto catalog = makeCatalog();
    auto rows = runQuery("SELECT age FROM users WHERE username = 'b'", catalog);
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_DOUBLE_EQ(std::get<double>(rows[0][0]), 19.0);
}

TEST(Planner, WhereStringLessThan) {
    auto catalog = makeCatalog();
    auto rows = runQuery("SELECT username FROM users WHERE username < 'c'", catalog);
    EXPECT_EQ(textColumn(rows, 0), (Names{"a", "b"}));
}

TEST(Planner, WhereWithStarKeepsAllColumns) {
    auto catalog = makeCatalog();
    auto rows = runQuery("SELECT * FROM users WHERE age >= 20", catalog);
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].size(), 3u);
    EXPECT_EQ(std::get<std::string>(rows[0][0]), "c");
}

TEST(Planner, WhereNoMatchesGivesNoRows) {
    auto catalog = makeCatalog();
    EXPECT_TRUE(runQuery("SELECT * FROM users WHERE age > 100", catalog).empty());
}

TEST(Planner, WhereColumnNotInSelect) {
    // Filter must run below Project: age is filtered on even though it isn't selected.
    auto catalog = makeCatalog();
    auto rows = runQuery("SELECT username FROM users WHERE age > 18", catalog);
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].size(), 1u);
}

TEST(Planner, WhereTypeMismatchGivesNoRows) {
    auto catalog = makeCatalog();
    EXPECT_TRUE(runQuery("SELECT * FROM users WHERE age = 'abc'", catalog).empty());
    EXPECT_TRUE(runQuery("SELECT * FROM users WHERE age != 'abc'", catalog).empty());
}

TEST(Planner, UnknownTableThrows) {
    auto catalog = makeCatalog();
    EXPECT_THROW(runQuery("SELECT * FROM nope", catalog), std::runtime_error);
}

TEST(Planner, UnknownSelectColumnThrows) {
    auto catalog = makeCatalog();
    EXPECT_THROW(runQuery("SELECT agee FROM users", catalog), std::runtime_error);
}

TEST(Planner, UnknownWhereColumnThrows) {
    auto catalog = makeCatalog();
    EXPECT_THROW(runQuery("SELECT * FROM users WHERE agee > 18", catalog), std::runtime_error);
}

TEST(Planner, ErrorMessageNamesTheColumn) {
    auto catalog = makeCatalog();
    try {
        runQuery("SELECT agee FROM users", catalog);
        FAIL() << "expected an exception";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("agee"), std::string::npos);
    }
}

TEST(Planner, ParseErrorPropagates) {
    auto catalog = makeCatalog();
    EXPECT_THROW(runQuery("SELECT FROM users", catalog), std::runtime_error);
}

static std::unique_ptr<Operator> planOnly(const std::string& sql,
                                          const std::map<std::string, Table>& catalog) {
    Parser parser(parseSQLquery(sql));
    return buildPlan(parser.parseSelect(), catalog);   // everything local to this function dies here
}

TEST(Planner, PlanOutlivesBuildPlanCall) {
    auto catalog = makeCatalog();
    auto plan = planOnly("SELECT username FROM users", catalog);
    Row row;
    ASSERT_TRUE(plan->next(row));                      // would read freed memory if Scan held a copy
    EXPECT_EQ(std::get<std::string>(row[0]), "a");
}

TEST(Planner, EmptyTableReturnsNoRows) {
    auto catalog = makeCatalog();
    catalog.at("users").rows.clear();
    EXPECT_TRUE(runQuery("SELECT * FROM users", catalog).empty());
}

TEST(Planner, TwoPlansOverSameTableAreIndependent) {
    auto catalog = makeCatalog();
    auto p1 = planOnly("SELECT username FROM users", catalog);
    auto p2 = planOnly("SELECT username FROM users", catalog);
    Row r1, r2;
    ASSERT_TRUE(p1->next(r1));
    ASSERT_TRUE(p1->next(r1));       // p1 advanced twice
    ASSERT_TRUE(p2->next(r2));       // p2 is still at the start
    EXPECT_EQ(std::get<std::string>(r2[0]), "a");
}

TEST(Planner, SelectStarWorksWithCapitalizedHeaders) {
    auto catalog = makeCapitalizedCatalog();
    auto rows = runQuery("SELECT * FROM people", catalog);
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].size(), 2u);
    EXPECT_EQ(std::get<std::string>(rows[0][0]), "a");
}

TEST(Planner, ColumnNamesAreCaseInsensitive) {
    auto catalog = makeCatalog();
    auto rows = runQuery("SELECT USERNAME FROM USERS WHERE AGE > 18", catalog);
    EXPECT_EQ(textColumn(rows, 0), (Names{"b", "c"}));
}

TEST(Planner, DecimalLiteralInWhere) {
    auto catalog = makeCatalog();
    catalog.at("users").rows[1][2] = 19.5;
    auto rows = runQuery("SELECT username FROM users WHERE age > 19.2", catalog);
    EXPECT_EQ(textColumn(rows, 0), (Names{"b", "c"}));
}

TEST(Planner, HugeNumberGivesCleanError) {
    auto catalog = makeCatalog();
    std::string sql = "SELECT * FROM users WHERE age > " + std::string(400, '9');
    EXPECT_THROW(runQuery(sql, catalog), std::runtime_error);
}