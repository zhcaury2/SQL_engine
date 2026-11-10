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

static Table makeFlags() {
    Table t;
    t.name = "flags";
    t.columns = {"id", "x", "y"};
    for (size_t i = 0; i < t.columns.size(); ++i) t.colIndex[t.columns[i]] = i;
    t.rows = {
        {std::string("r00"), 0.0, 0.0},
        {std::string("r01"), 0.0, 1.0},
        {std::string("r10"), 1.0, 0.0},
        {std::string("r11"), 1.0, 1.0},
    };
    return t;
}

static Names runPred(const Table& t, std::unique_ptr<Predicate> p) {
    Filter f(std::make_unique<Scan>(t), std::move(p));
    Names out;
    Row row;
    while (f.next(row)) out.push_back(std::get<std::string>(row[0]));
    return out;
}

static std::unique_ptr<Predicate> xIs1() { return makeCompare(1, "=", Value(1.0)); }
static std::unique_ptr<Predicate> yIs1() { return makeCompare(2, "=", Value(1.0)); }


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


TEST(AndOrPredicate, AndTruthTable) {
    Table t = makeFlags();
    EXPECT_EQ(runPred(t, makeAnd(xIs1(), yIs1())), (Names{"r11"}));
}

TEST(AndOrPredicate, OrTruthTable) {
    Table t = makeFlags();
    EXPECT_EQ(runPred(t, makeOr(xIs1(), yIs1())), (Names{"r01", "r10", "r11"}));
}

TEST(AndOrPredicate, AndIsSymmetric) {
    Table t = makeFlags();
    EXPECT_EQ(runPred(t, makeAnd(xIs1(), yIs1())),
              runPred(t, makeAnd(yIs1(), xIs1())));
}

TEST(AndOrPredicate, OrIsSymmetric) {
    Table t = makeFlags();
    EXPECT_EQ(runPred(t, makeOr(xIs1(), yIs1())),
              runPred(t, makeOr(yIs1(), xIs1())));
}

TEST(AndOrPredicate, TreeShapeChangesResult) {
    Table t = makeFlags();
    // x=1 OR (y=1 AND id='r01')  -> r10, r11 (x=1) plus r01 (y=1 and id matches)
    auto p1 = makeOr(xIs1(), makeAnd(yIs1(), makeCompare(0, "=", Value(std::string("r01")))));
    EXPECT_EQ(runPred(t, std::move(p1)), (Names{"r01", "r10", "r11"}));

    // (x=1 OR y=1) AND id='r01'  -> only r01
    auto p2 = makeAnd(makeOr(xIs1(), yIs1()), makeCompare(0, "=", Value(std::string("r01"))));
    EXPECT_EQ(runPred(t, std::move(p2)), (Names{"r01"}));
}

TEST(AndOrPredicate, ThreeWayChains) {
    Table t = makeFlags();
    auto orChain = makeOr(makeOr(makeCompare(0, "=", Value(std::string("r00"))),
                                 makeCompare(0, "=", Value(std::string("r01")))),
                          makeCompare(0, "=", Value(std::string("r10"))));
    EXPECT_EQ(runPred(t, std::move(orChain)), (Names{"r00", "r01", "r10"}));

    auto andChain = makeAnd(makeAnd(makeCompare(1, ">=", Value(0.0)),
                                    makeCompare(2, ">=", Value(0.0))),
                            makeCompare(0, "!=", Value(std::string("r00"))));
    EXPECT_EQ(runPred(t, std::move(andChain)), (Names{"r01", "r10", "r11"}));
}

TEST(AndOrPredicate, RangeOnOneColumn) {
    Table t = makeUsers();   // ages 18, 19, 20
    auto between = makeAnd(makeCompare(2, ">=", Value(19.0)), makeCompare(2, "<=", Value(19.0)));
    EXPECT_EQ(runPred(t, std::move(between)), (Names{"b"}));
}

TEST(AndOrPredicate, ContradictionIsEmpty) {
    Table t = makeUsers();
    auto p = makeAnd(makeCompare(2, ">", Value(19.0)), makeCompare(2, "<", Value(19.0)));
    EXPECT_TRUE(runPred(t, std::move(p)).empty());
}

TEST(AndOrPredicate, TautologyMatchesEverything) {
    Table t = makeUsers();
    auto p = makeOr(makeCompare(2, "<=", Value(19.0)), makeCompare(2, ">", Value(19.0)));
    EXPECT_EQ(runPred(t, std::move(p)), (Names{"a", "b", "c"}));
}

TEST(AndOrPredicate, NullFailsAndButOrCanRescueIt) {
    Table t = makeUsers();
    t.rows[1][2] = std::monostate{};     // b's age is NULL

    // NULL > 18 is false, but username = 'b' still holds
    auto orP = makeOr(makeCompare(2, ">", Value(18.0)),
                      makeCompare(0, "=", Value(std::string("b"))));
    EXPECT_EQ(runPred(t, std::move(orP)), (Names{"b", "c"}));

    // NULL != 18 is false, so the AND fails even though username = 'b' holds
    auto andP = makeAnd(makeCompare(2, "!=", Value(18.0)),
                        makeCompare(0, "=", Value(std::string("b"))));
    EXPECT_TRUE(runPred(t, std::move(andP)).empty());
}

TEST(AndOrPredicate, TypeMismatchSideDoesNotMatchButOrStillWorks) {
    Table t = makeUsers();
    auto p = makeOr(makeCompare(2, "=", Value(std::string("abc"))),   // number vs string: false
                    makeCompare(0, "=", Value(std::string("a"))));
    EXPECT_EQ(runPred(t, std::move(p)), (Names{"a"}));
}

TEST(AndOrPredicate, MissingOperandThrowsWhenBuilt) {
    EXPECT_THROW(makeAnd(nullptr, xIs1()), std::runtime_error);
    EXPECT_THROW(makeOr(xIs1(), nullptr), std::runtime_error);
}

TEST(AndOrPredicate, FilterOverCompoundKeepsFullRow) {
    Table t = makeUsers();
    Filter f(std::make_unique<Scan>(t),
             makeAnd(makeCompare(2, ">", Value(18.0)), makeCompare(0, "<", Value(std::string("c")))));
    Row row;
    ASSERT_TRUE(f.next(row));
    ASSERT_EQ(row.size(), 3u);
    EXPECT_EQ(std::get<std::string>(row[1]), "b2");
    EXPECT_FALSE(f.next(row));
}