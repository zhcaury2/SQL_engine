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

static Table makeWords() {
    Table t;
    t.name = "words";
    t.columns = {"id", "genre", "n"};
    for (size_t i = 0; i < t.columns.size(); ++i) t.colIndex[t.columns[i]] = i;
    t.rows = {
        {std::string("r1"), std::string("Drama"),              1.0},
        {std::string("r2"), std::string("Crime;Drama"),        2.0},
        {std::string("r3"), std::string("Action;Crime;Drama"), 3.0},
        {std::string("r4"), std::string("Comedy"),             4.0},
        {std::string("r5"), std::string("drama"),              5.0},
        {std::string("r6"), std::monostate{},                  6.0},
        {std::string("r7"), 42.0,                              7.0},
        {std::string("r8"), std::string(""),                   8.0},
    };
    return t;
}

static std::unique_ptr<Predicate> like(const std::string& pattern) {
    return makeCompare(1, "LIKE", Value(pattern));
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


static const bool kAsc = false;
static const bool kDesc = true;

static Names drainIds(Operator& op) {
    Names out;
    Row row;
    while (op.next(row)) out.push_back(std::get<std::string>(row[0]));
    return out;
}

static Names sortedIds(const Table& t, std::vector<SortKey> keys) {
    Sort s(std::make_unique<Scan>(t), std::move(keys));
    return drainIds(s);
}

static Table makeSortable() {
    Table t;
    t.name = "sortable";
    t.columns = {"id", "n", "s"};
    for (size_t i = 0; i < t.columns.size(); ++i) t.colIndex[t.columns[i]] = i;
    t.rows = {
        {std::string("r1"), 3.0, std::string("b")},
        {std::string("r2"), 1.0, std::string("a")},
        {std::string("r3"), 2.0, std::string("c")},
        {std::string("r4"), 1.0, std::string("b")},
        {std::string("r5"), 2.0, std::string("a")},
    };
    return t;
}

class CountingSource : public Operator {
    std::vector<Row> rows;
    size_t pos = 0;
public:
    size_t pulls = 0;
    explicit CountingSource(std::vector<Row> r) : rows(std::move(r)) {}
    bool next(Row& out) override {
        ++pulls;
        if (pos >= rows.size()) return false;
        out = rows[pos++];
        return true;
    }
};

static std::vector<Row> fourRows() {
    return {{std::string("a")}, {std::string("b")}, {std::string("c")}, {std::string("d")}};
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

TEST(LimitOp, ReturnsFirstNRowsInOrder) {
    Table t = makeUsers();
    Limit l(std::make_unique<Scan>(t), 2);
    EXPECT_EQ(drainIds(l), (Names{"a", "b"}));
}

TEST(LimitOp, ZeroReturnsNothing) {
    Table t = makeUsers();
    Limit l(std::make_unique<Scan>(t), 0);
    Row row;
    EXPECT_FALSE(l.next(row));
}

TEST(LimitOp, LargerThanInputReturnsEverything) {
    Table t = makeUsers();
    Limit l(std::make_unique<Scan>(t), 100);
    EXPECT_EQ(drainIds(l), (Names{"a", "b", "c"}));
}

TEST(LimitOp, ExactlyInputSize) {
    Table t = makeUsers();
    Limit l(std::make_unique<Scan>(t), 3);
    EXPECT_EQ(drainIds(l), (Names{"a", "b", "c"}));
}

TEST(LimitOp, EmptyInput) {
    Table t = makeUsers();
    t.rows.clear();
    Limit l(std::make_unique<Scan>(t), 5);
    Row row;
    EXPECT_FALSE(l.next(row));
}

TEST(LimitOp, StaysExhausted) {
    Table t = makeUsers();
    Limit l(std::make_unique<Scan>(t), 1);
    Row row;
    ASSERT_TRUE(l.next(row));
    EXPECT_FALSE(l.next(row));
    EXPECT_FALSE(l.next(row));
}

TEST(LimitOp, RowsAreUnchanged) {
    Table t = makeUsers();
    Limit l(std::make_unique<Scan>(t), 1);
    Row row;
    ASSERT_TRUE(l.next(row));
    ASSERT_EQ(row.size(), 3u);
    EXPECT_EQ(std::get<std::string>(row[1]), "a2");
    EXPECT_DOUBLE_EQ(std::get<double>(row[2]), 18.0);
}

TEST(LimitOp, StopsPullingFromChildOnceSatisfied) {
    auto src = std::make_unique<CountingSource>(fourRows());
    CountingSource* raw = src.get();          // stays valid while `l` owns the source
    Limit l(std::move(src), 2);
    Row row;
    ASSERT_TRUE(l.next(row));
    ASSERT_TRUE(l.next(row));
    EXPECT_EQ(raw->pulls, 2u);
    EXPECT_FALSE(l.next(row));
    EXPECT_EQ(raw->pulls, 2u);                // the third call must not touch the child
}

TEST(LimitOp, ZeroNeverPullsFromChild) {
    auto src = std::make_unique<CountingSource>(fourRows());
    CountingSource* raw = src.get();
    Limit l(std::move(src), 0);
    Row row;
    EXPECT_FALSE(l.next(row));
    EXPECT_EQ(raw->pulls, 0u);
}

TEST(LimitOp, CountsRowsThatPassAFilterBelowIt) {
    Table t = makeUsers();                    // ages 18, 19, 20
    Limit l(std::make_unique<Filter>(std::make_unique<Scan>(t), makeCompare(2, ">=", Value(19.0))), 1);
    EXPECT_EQ(drainIds(l), (Names{"b"}));
}

TEST(SortOp, AscendingNumbersWithStableTies) {
    Table t = makeSortable();
    EXPECT_EQ(sortedIds(t, {{1, kAsc}}), (Names{"r2", "r4", "r3", "r5", "r1"}));
}

TEST(SortOp, DescendingNumbersTiesKeepOriginalOrder) {
    Table t = makeSortable();
    // DESC must not simply reverse the ASC result: r3 still comes before r5.
    EXPECT_EQ(sortedIds(t, {{1, kDesc}}), (Names{"r1", "r3", "r5", "r2", "r4"}));
}

TEST(SortOp, AscendingStrings) {
    Table t = makeSortable();
    EXPECT_EQ(sortedIds(t, {{2, kAsc}}), (Names{"r2", "r5", "r1", "r4", "r3"}));
}

TEST(SortOp, DescendingStrings) {
    Table t = makeSortable();
    EXPECT_EQ(sortedIds(t, {{2, kDesc}}), (Names{"r3", "r1", "r4", "r2", "r5"}));
}

TEST(SortOp, SortByTextIdBothDirections) {
    Table t = makeSortable();
    EXPECT_EQ(sortedIds(t, {{0, kAsc}}),  (Names{"r1", "r2", "r3", "r4", "r5"}));
    EXPECT_EQ(sortedIds(t, {{0, kDesc}}), (Names{"r5", "r4", "r3", "r2", "r1"}));
}

TEST(SortOp, TwoKeysSecondBreaksTies) {
    Table t = makeSortable();
    // n ASC, s DESC
    EXPECT_EQ(sortedIds(t, {{1, kAsc}, {2, kDesc}}), (Names{"r4", "r2", "r3", "r5", "r1"}));
    // n DESC, s ASC
    EXPECT_EQ(sortedIds(t, {{1, kDesc}, {2, kAsc}}), (Names{"r1", "r5", "r3", "r2", "r4"}));
}

TEST(SortOp, KeyOrderMatters) {
    Table t = makeSortable();
    // s ASC, n DESC  differs from  n DESC, s ASC
    EXPECT_EQ(sortedIds(t, {{2, kAsc}, {1, kDesc}}), (Names{"r5", "r2", "r1", "r4", "r3"}));
}

TEST(SortOp, RepeatedKeyIsHarmless) {
    Table t = makeSortable();
    EXPECT_EQ(sortedIds(t, {{1, kAsc}, {1, kAsc}}), (Names{"r2", "r4", "r3", "r5", "r1"}));
}

TEST(SortOp, NumbersCompareNumericallyNotAlphabetically) {
    Table t;
    t.columns = {"id", "n"};
    t.rows = {
        {std::string("ten"), 10.0},
        {std::string("nine"), 9.0},
        {std::string("hundred"), 100.0},
    };
    EXPECT_EQ(sortedIds(t, {{1, kAsc}}), (Names{"nine", "ten", "hundred"}));
}

TEST(SortOp, NegativeAndDecimalNumbers) {
    Table t;
    t.columns = {"id", "n"};
    t.rows = {
        {std::string("a"), 2.25},
        {std::string("b"), -1.5},
        {std::string("c"), 0.0},
    };
    EXPECT_EQ(sortedIds(t, {{1, kAsc}}), (Names{"b", "c", "a"}));
}

TEST(SortOp, StringsCompareByteWise) {
    Table t;
    t.columns = {"id"};
    t.rows = {{std::string("b")}, {std::string("A")}, {std::string("a")}, {std::string("B")}};
    EXPECT_EQ(sortedIds(t, {{0, kAsc}}), (Names{"A", "B", "a", "b"}));
}

TEST(SortOp, RowsComeBackIntact) {
    Table t = makeSortable();
    Sort s(std::make_unique<Scan>(t), {{1, kDesc}});
    Row row;
    ASSERT_TRUE(s.next(row));
    ASSERT_EQ(row.size(), 3u);
    EXPECT_EQ(std::get<std::string>(row[0]), "r1");
    EXPECT_DOUBLE_EQ(std::get<double>(row[1]), 3.0);
    EXPECT_EQ(std::get<std::string>(row[2]), "b");
}

TEST(SortOp, EmptyInput) {
    Table t = makeSortable();
    t.rows.clear();
    Sort s(std::make_unique<Scan>(t), {{1, kAsc}});
    Row row;
    EXPECT_FALSE(s.next(row));
}

TEST(SortOp, SingleRow) {
    Table t = makeSortable();
    t.rows.resize(1);
    EXPECT_EQ(sortedIds(t, {{1, kDesc}}), (Names{"r1"}));
}

TEST(SortOp, AlreadySortedInputIsUnchanged) {
    Table t = makeUsers();                    // ages already ascending
    EXPECT_EQ(sortedIds(t, {{2, kAsc}}), (Names{"a", "b", "c"}));
}

TEST(SortOp, StaysExhausted) {
    Table t = makeSortable();
    Sort s(std::make_unique<Scan>(t), {{1, kAsc}});
    Row row;
    for (int i = 0; i < 5; ++i) ASSERT_TRUE(s.next(row));
    EXPECT_FALSE(s.next(row));
    EXPECT_FALSE(s.next(row));
}

TEST(SortOp, SourceTableIsNotReordered) {
    Table t = makeSortable();
    sortedIds(t, {{1, kDesc}});
    EXPECT_EQ(std::get<std::string>(t.rows[0][0]), "r1");
    EXPECT_EQ(std::get<std::string>(t.rows[4][0]), "r5");
}

TEST(SortOp, EmptyKeyListThrows) {
    Table t = makeSortable();
    EXPECT_THROW(Sort(std::make_unique<Scan>(t), std::vector<SortKey>{}), std::runtime_error);
}

TEST(SortOp, ReadsAllInputOnFirstNextAndNeverAgain) {
    auto src = std::make_unique<CountingSource>(fourRows());
    CountingSource* raw = src.get();
    Sort s(std::move(src), {{0, kDesc}});
    EXPECT_EQ(raw->pulls, 0u);                // construction must not pull anything
    Row row;
    ASSERT_TRUE(s.next(row));
    EXPECT_GE(raw->pulls, 4u);                // blocking: the whole input was consumed
    size_t afterFirst = raw->pulls;
    while (s.next(row)) {}
    EXPECT_EQ(raw->pulls, afterFirst);        // later calls come from the sorted buffer
}

TEST(SortOp, NullSortsFirstAscendingAndLastDescending) {
    Table t = makeSortable();
    t.rows[2][1] = std::monostate{};          // r3.n is NULL
    EXPECT_EQ(sortedIds(t, {{1, kAsc}}),  (Names{"r3", "r2", "r4", "r5", "r1"}));
    EXPECT_EQ(sortedIds(t, {{1, kDesc}}), (Names{"r1", "r5", "r2", "r4", "r3"}));
}

TEST(SortOp, MultipleNullsKeepOriginalOrder) {
    Table t = makeSortable();
    t.rows[2][1] = std::monostate{};          // r3
    t.rows[4][1] = std::monostate{};          // r5
    EXPECT_EQ(sortedIds(t, {{1, kAsc}}),  (Names{"r3", "r5", "r2", "r4", "r1"}));
    EXPECT_EQ(sortedIds(t, {{1, kDesc}}), (Names{"r1", "r2", "r4", "r3", "r5"}));
}

TEST(SortOp, NullTiesFallThroughToSecondKey) {
    Table t = makeSortable();
    t.rows[2][1] = std::monostate{};          // r3 (s = c)
    t.rows[4][1] = std::monostate{};          // r5 (s = a)
    // n ASC, s ASC: the two NULLs tie on n, so s decides: r5 (a) before r3 (c)
    EXPECT_EQ(sortedIds(t, {{1, kAsc}, {2, kAsc}}), (Names{"r5", "r3", "r2", "r4", "r1"}));
}

TEST(SortOp, AllNullColumnKeepsOriginalOrder) {
    Table t = makeSortable();
    for (auto& r : t.rows) r[1] = std::monostate{};
    EXPECT_EQ(sortedIds(t, {{1, kAsc}}), (Names{"r1", "r2", "r3", "r4", "r5"}));
}

TEST(SortOp, MixedTypesOrderNumbersBeforeStrings) {
    Table t = makeSortable();
    t.rows[1][1] = std::string("x");          // r2.n is text in a numeric column
    EXPECT_EQ(sortedIds(t, {{1, kAsc}}),  (Names{"r4", "r3", "r5", "r1", "r2"}));
    EXPECT_EQ(sortedIds(t, {{1, kDesc}}), (Names{"r2", "r1", "r3", "r5", "r4"}));
}

TEST(SortOp, TopNWithLimitAbove) {
    Table t = makeSortable();
    Limit l(std::make_unique<Sort>(std::make_unique<Scan>(t), std::vector<SortKey>{{1, kDesc}}), 2);
    EXPECT_EQ(drainIds(l), (Names{"r1", "r3"}));
}

TEST(SortOp, SortsOnlyRowsThatPassedTheFilter) {
    Table t = makeSortable();
    auto filtered = std::make_unique<Filter>(std::make_unique<Scan>(t), makeCompare(1, ">=", Value(2.0)));
    Sort s(std::move(filtered), {{2, kAsc}});
    EXPECT_EQ(drainIds(s), (Names{"r5", "r1", "r3"}));
}

TEST(SortOp, ProjectAboveSortStillSeesSortedOrder) {
    Table t = makeSortable();
    Project p(std::make_unique<Sort>(std::make_unique<Scan>(t), std::vector<SortKey>{{1, kDesc}}),
              {0});
    Row row;
    ASSERT_TRUE(p.next(row));
    ASSERT_EQ(row.size(), 1u);
    EXPECT_EQ(std::get<std::string>(row[0]), "r1");
}

struct LikeCase { const char* text; const char* pattern; bool expected; };

static void checkCases(const std::vector<LikeCase>& cases) {
    for (const auto& c : cases) {
        EXPECT_EQ(likeMatch(c.text, c.pattern), c.expected)
            << "text='" << c.text << "' pattern='" << c.pattern << "'";
    }
}

TEST(LikeMatch, ExactMatchesWithoutWildcards) {
    checkCases({
        {"abc", "abc", true},  {"abc", "abd", false},
        {"abc", "ab", false},  {"ab", "abc", false},
        {"", "", true},        {"a", "", false},     {"", "a", false},
    });
}

TEST(LikeMatch, IsCaseSensitive) {
    checkCases({{"ABC", "abc", false}, {"abc", "ABC", false}, {"Drama", "Drama", true}});
}

TEST(LikeMatch, UnderscoreMatchesExactlyOneCharacter) {
    checkCases({
        {"a", "_", true},      {"", "_", false},      {"ab", "_", false},
        {"abc", "a_c", true},  {"ac", "a_c", false},
        {"abc", "___", true},  {"abc", "__", false},  {"abc", "____", false},
        {"a b", "a_b", true},  {"a\nb", "a_b", true}, {"a\tb", "a_b", true},
    });
}

TEST(LikeMatch, PercentMatchesAnyRunIncludingEmpty) {
    checkCases({
        {"", "%", true},       {"abc", "%", true},    {"abc", "%%", true},
        {"abc", "%%%", true},  {"", "%%", true},
        {"abc", "a%", true},   {"abc", "%c", true},   {"abc", "%b%", true},
        {"abc", "a%c", true},  {"ac", "a%c", true},
        {"abc", "a%d", false}, {"abc", "%d", false},  {"abc", "d%", false},
        {"a", "a%a", false},   {"aa", "a%a", true},
        {"abc", "%abc%", true}, {"abc", "%abcd%", false},
    });
}

TEST(LikeMatch, WildcardsCombine) {
    checkCases({
        {"abc", "_%", true},   {"", "_%", false},
        {"a", "_%_", false},   {"ab", "_%_", true},
        {"abc", "%_c", true},  {"c", "%_c", false},
        {"ac", "_%c", true},   {"c", "_%c", false},
        {"abc", "%_", true},   {"abc", "a_%", true},  {"a", "a_%", false},
    });
}

TEST(LikeMatch, NeedsBacktrackingToFindTheRightSplit) {
    checkCases({
        {"aab", "%ab", true},            {"abab", "%ab", true},
        {"abcabd", "%abd", true},        {"abcabd", "a%abd", true},
        {"mississippi", "%iss%ppi", true},
        {"mississippi", "%ssip%", true},
        {"mississippi", "%sss%", false},
        {"mississippi", "m%iss%pi", true},
        {"mississippi", "m%iss%pix", false},
        {"aaa", "a%a%a", true},          {"aa", "a%a%a", false},
        {"abcbc", "%bc", true},          {"abxabxab", "%abxab", true},
    });
}

TEST(LikeMatch, OtherCharactersAreLiteral) {
    // No regex behavior and no escape character.
    checkCases({
        {"a.c", "a.c", true},   {"abc", "a.c", false},
        {"a*c", "a*c", true},   {"abc", "a*c", false},
        {"[a]", "[a]", true},   {"a", "[a]", false},
        {"(a)", "(a)", true},   {"a+b", "a+b", true},  {"x^y", "x^y", true},
        {"a\\b", "a\\b", true},               // backslash is an ordinary character
    });
}

TEST(LikeMatch, PercentAndUnderscoreInTheTextAreJustCharacters) {
    checkCases({
        {"50%", "50%", true},   {"500", "50%", true},
        {"50%", "50_", true},   {"5_0", "5_0", true},  {"5x0", "5_0", true},
    });
}

TEST(LikeMatch, WhitespaceIsSignificant) {
    checkCases({{"abc ", "abc", false}, {"abc", "abc ", false}, {"abc ", "abc%", true}});
}

TEST(LikeMatch, MatchesBytesNotCharacters) {
    // Documented limitation: "é" is two bytes in UTF-8.
    EXPECT_FALSE(likeMatch("\xC3\xA9", "_"));
    EXPECT_TRUE(likeMatch("\xC3\xA9", "__"));
    EXPECT_TRUE(likeMatch("caf\xC3\xA9", "caf%"));
}

TEST(LikeMatch, ManyWildcardsDoNotExplode) {
    // A naive recursive matcher takes exponential time on this input
    // (the test would appear to hang). A two-pointer matcher with one
    // backtrack point finishes instantly.
    std::string text(60, 'a');
    EXPECT_FALSE(likeMatch(text, "%a%a%a%a%a%a%a%a%a%a%b"));
    EXPECT_TRUE(likeMatch(text, "%a%a%a%a%a%a%a%a%a%a%"));
}

TEST(LikePredicate, ContainsPattern) {
    Table t = makeWords();
    EXPECT_EQ(runPred(t, like("%Drama%")), (Names{"r1", "r2", "r3"}));
}

TEST(LikePredicate, PrefixAndSuffixPatterns) {
    Table t = makeWords();
    EXPECT_EQ(runPred(t, like("Crime%")), (Names{"r2"}));
    EXPECT_EQ(runPred(t, like("%Drama")),  (Names{"r1", "r2", "r3"}));
    EXPECT_EQ(runPred(t, like("%;%")),     (Names{"r2", "r3"}));
}

TEST(LikePredicate, NoWildcardsMeansExactMatch) {
    Table t = makeWords();
    EXPECT_EQ(runPred(t, like("Drama")), (Names{"r1"}));
}

TEST(LikePredicate, UnderscoreMatchesOneCharacter) {
    Table t = makeWords();
    EXPECT_EQ(runPred(t, like("Dram_")), (Names{"r1"}));
    EXPECT_EQ(runPred(t, like("_____")), (Names{"r1", "r5"}));   // five characters
    EXPECT_TRUE(runPred(t, like("Dram")).empty());
}

TEST(LikePredicate, MatchesAreCaseSensitive) {
    Table t = makeWords();
    EXPECT_EQ(runPred(t, like("%drama%")), (Names{"r5"}));
}

TEST(LikePredicate, PercentAloneMatchesEveryStringButNotNullOrNumbers) {
    Table t = makeWords();
    // r6 is NULL and r7 is a number, so neither appears
    EXPECT_EQ(runPred(t, like("%")), (Names{"r1", "r2", "r3", "r4", "r5", "r8"}));
}

TEST(LikePredicate, EmptyPatternMatchesOnlyTheEmptyString) {
    Table t = makeWords();
    EXPECT_EQ(runPred(t, like("")), (Names{"r8"}));
}

TEST(LikePredicate, NumberCellsNeverMatchEvenIfTheDigitsDo) {
    Table t = makeWords();
    EXPECT_TRUE(runPred(t, like("4%")).empty());      // r7 holds 42.0
    EXPECT_TRUE(runPred(t, like("42")).empty());
}

TEST(LikePredicate, NullCellNeverMatches) {
    Table t = makeWords();
    EXPECT_EQ(runPred(t, makeOr(like("%Comedy%"), makeCompare(0, "=", Value(std::string("r6"))))),
              (Names{"r4", "r6"}));                   // r6 is only there because of the other side
}

TEST(LikePredicate, CombinesWithAnd) {
    Table t = makeWords();
    EXPECT_EQ(runPred(t, makeAnd(like("%Drama%"), like("Crime%"))), (Names{"r2"}));
    EXPECT_EQ(runPred(t, makeAnd(like("%Drama%"), makeCompare(2, ">", Value(2.0)))), (Names{"r3"}));
}

TEST(LikePredicate, CombinesWithOr) {
    Table t = makeWords();
    EXPECT_EQ(runPred(t, makeOr(like("Comedy"), like("Action%"))), (Names{"r3", "r4"}));
}

TEST(LikePredicate, PercentInTheDataIsJustACharacter) {
    Table t;
    t.columns = {"id", "t"};
    t.rows = {
        {std::string("a"), std::string("50%")},
        {std::string("b"), std::string("500")},
        {std::string("c"), std::string("5")},
    };
    EXPECT_EQ(runPred(t, makeCompare(1, "LIKE", Value(std::string("50%")))), (Names{"a", "b"}));
    EXPECT_EQ(runPred(t, makeCompare(1, "LIKE", Value(std::string("50_")))), (Names{"a", "b"}));
    EXPECT_EQ(runPred(t, makeCompare(1, "LIKE", Value(std::string("5%")))),  (Names{"a", "b", "c"}));
}

TEST(LikePredicate, EmptyTable) {
    Table t = makeWords();
    t.rows.clear();
    EXPECT_TRUE(runPred(t, like("%")).empty());
}

TEST(LikePredicate, ResultsKeepTableOrder) {
    Table t = makeWords();
    auto r = runPred(t, like("%Drama%"));
    EXPECT_TRUE(std::is_sorted(r.begin(), r.end()));
}

TEST(LikePredicate, NonStringPatternThrowsWhenBuilt) {
    EXPECT_THROW(makeCompare(1, "LIKE", Value(5.0)), std::runtime_error);
    EXPECT_THROW(makeCompare(1, "LIKE", Value(std::monostate{})), std::runtime_error);
}