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

static std::vector<std::string> runFilter(const Table& table, size_t col,
                                          const std::string& op, Value literal) {
    Filter f(std::make_unique<Scan>(table), col, op, std::move(literal));
    std::vector<std::string> names;
    Row row;
    while (f.next(row)) names.push_back(std::get<std::string>(row[0]));
    return names;
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

TEST(Filter, NumberGreaterThan) {
    Table t = makeUsers();
    EXPECT_EQ(runFilter(t, 2, ">", 18.0), (Names{"b", "c"}));
}

TEST(Filter, NumberGreaterOrEqual) {
    Table t = makeUsers();
    EXPECT_EQ(runFilter(t, 2, ">=", 19.0), (Names{"b", "c"}));
}

TEST(Filter, NumberLessThan) {
    Table t = makeUsers();
    EXPECT_EQ(runFilter(t, 2, "<", 19.0), (Names{"a"}));
}

TEST(Filter, NumberLessOrEqual) {
    Table t = makeUsers();
    EXPECT_EQ(runFilter(t, 2, "<=", 19.0), (Names{"a", "b"}));
}

TEST(Filter, NumberEquals) {
    Table t = makeUsers();
    EXPECT_EQ(runFilter(t, 2, "=", 19.0), (Names{"b"}));
}

TEST(Filter, NumberNotEquals) {
    Table t = makeUsers();
    EXPECT_EQ(runFilter(t, 2, "!=", 19.0), (Names{"a", "c"}));
}

TEST(Filter, StringEquals) {
    Table t = makeUsers();
    EXPECT_EQ(runFilter(t, 0, "=", std::string("c")), (Names{"c"}));
}

TEST(Filter, StringNotEquals) {
    Table t = makeUsers();
    EXPECT_EQ(runFilter(t, 0, "!=", std::string("b")), (Names{"a", "c"}));
}

TEST(Filter, StringLessThan) {
    Table t = makeUsers();
    EXPECT_EQ(runFilter(t, 0, "<", std::string("b")), (Names{"a"}));
}

TEST(Filter, StringGreaterOrEqual) {
    Table t = makeUsers();
    EXPECT_EQ(runFilter(t, 0, ">=", std::string("b")), (Names{"b", "c"}));
}

TEST(Filter, NoMatchesReturnsFalseImmediately) {
    Table t = makeUsers();
    Filter f(std::make_unique<Scan>(t), 2, ">", Value(100.0));
    Row row;
    EXPECT_FALSE(f.next(row));
}

TEST(Filter, AllRowsMatch) {
    Table t = makeUsers();
    EXPECT_EQ(runFilter(t, 2, ">", 0.0), (Names{"a", "b", "c"}));
}

TEST(Filter, EmptyTable) {
    Table t = makeUsers();
    t.rows.clear();
    EXPECT_TRUE(runFilter(t, 2, ">", 0.0).empty());
}

TEST(Filter, StaysExhausted) {
    Table t = makeUsers();
    Filter f(std::make_unique<Scan>(t), 2, "=", Value(20.0));
    Row row;
    ASSERT_TRUE(f.next(row));
    EXPECT_FALSE(f.next(row));
    EXPECT_FALSE(f.next(row));
}

TEST(Filter, ReturnsFullUnchangedRow) {
    Table t = makeUsers();
    Filter f(std::make_unique<Scan>(t), 2, "=", Value(19.0));
    Row row;
    ASSERT_TRUE(f.next(row));
    ASSERT_EQ(row.size(), 3u);
    EXPECT_EQ(std::get<std::string>(row[0]), "b");
    EXPECT_EQ(std::get<std::string>(row[1]), "b2");
    EXPECT_DOUBLE_EQ(std::get<double>(row[2]), 19.0);
}

TEST(Filter, SkipsSeveralRowsInOneCall) {
    // Only the last row matches, so one next() must consume all three child rows.
    Table t = makeUsers();
    Filter f(std::make_unique<Scan>(t), 2, "=", Value(20.0));
    Row row;
    ASSERT_TRUE(f.next(row));
    EXPECT_EQ(std::get<std::string>(row[0]), "c");
    EXPECT_FALSE(f.next(row));
}

TEST(Filter, NullNeverMatchesAnyOperator) {
    Table t = makeUsers();
    t.rows[1][2] = std::monostate{};          // b's age is NULL
    EXPECT_EQ(runFilter(t, 2, ">", 0.0),  (Names{"a", "c"}));
    EXPECT_EQ(runFilter(t, 2, "!=", 18.0), (Names{"c"}));   // NULL must NOT pass !=
    EXPECT_EQ(runFilter(t, 2, "=", 19.0),  (Names{}));
}

TEST(Filter, NumberColumnVsStringLiteralNeverMatches) {
    Table t = makeUsers();
    EXPECT_TRUE(runFilter(t, 2, "=", std::string("abc")).empty());
    EXPECT_TRUE(runFilter(t, 2, "!=", std::string("abc")).empty());   // mismatch is not "unequal"
}

TEST(Filter, TextColumnVsNumberLiteralNeverMatches) {
    Table t = makeUsers();
    EXPECT_TRUE(runFilter(t, 0, "=", 5.0).empty());
    EXPECT_TRUE(runFilter(t, 0, "!=", 5.0).empty());
}

TEST(Filter, TwoFiltersStackLikeAnd) {
    // age >= 19 AND username < 'c'  ->  only b
    Table t = makeUsers();
    auto inner = std::make_unique<Filter>(std::make_unique<Scan>(t), 2, ">=", Value(19.0));
    Filter outer(std::move(inner), 0, "<", Value(std::string("c")));
    Row row;
    ASSERT_TRUE(outer.next(row));
    EXPECT_EQ(std::get<std::string>(row[0]), "b");
    EXPECT_FALSE(outer.next(row));
}

TEST(Filter, UnknownOperatorThrowsAtConstruction) {
    Table t = makeUsers();
    EXPECT_THROW(Filter(std::make_unique<Scan>(t), 2, "~", Value(1.0)), std::runtime_error);
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

