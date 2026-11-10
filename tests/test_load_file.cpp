#include "load_file.h"
#include <filesystem>
#include <fstream>
#include <variant>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

struct TempCsv {
    fs::path path;
    explicit TempCsv(const std::string& contents, const std::string& name = "test_tmp.csv") {
        path = fs::temp_directory_path() / name;
        std::ofstream out(path, std::ios::binary);
        out << contents;
    }
    ~TempCsv() { fs::remove(path); }
    std::string str() const { return path.string(); }
};

TEST(Loader, HeaderBecomesColumnsAndIndex) {
    TempCsv f("username,password,age\na,a2,18\n");
    Table t = loadCsv(f.str(), "users");
    ASSERT_EQ(t.columns.size(), 3u);
    EXPECT_EQ(t.columns[0], "username");
    EXPECT_EQ(t.colIndex.at("age"), 2u);
    EXPECT_EQ(t.colIndex.at("username"), 0u);
}

TEST(Loader, HeaderIsNotADataRow) {
    TempCsv f("x,y\n1,2\n3,4\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_EQ(t.rows.size(), 2u);
}

TEST(Loader, TableNameIsStored) {
    TempCsv f("x\n1\n");
    EXPECT_EQ(loadCsv(f.str(), "mytable").name, "mytable");   // use .Name if that's your field
}

TEST(Loader, HeaderOnlyFileHasZeroRows) {
    TempCsv f("x,y\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_EQ(t.columns.size(), 2u);
    EXPECT_TRUE(t.rows.empty());
}

TEST(Loader, MixedColumnStaysText) {
    // "x" disqualifies the column, so even "1" must stay a string
    TempCsv f("v\n1\nx\n3\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_TRUE(std::holds_alternative<std::string>(t.rows[0][0]));
    EXPECT_EQ(std::get<std::string>(t.rows[0][0]), "1");
}

TEST(Loader, DecimalsAndNegativesAreNumbers) {
    TempCsv f("v\n-5\n3.14\n0\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_DOUBLE_EQ(std::get<double>(t.rows[0][0]), -5.0);
    EXPECT_DOUBLE_EQ(std::get<double>(t.rows[1][0]), 3.14);
}

TEST(Loader, NumberWithTrailingJunkIsText) {
    TempCsv f("v\n12abc\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_TRUE(std::holds_alternative<std::string>(t.rows[0][0]));
}

TEST(Loader, NumericValueKeepsExactValue) {
    TempCsv f("age\n18\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_EQ(std::get<double>(t.rows[0][0]), 18.0);
}

TEST(Loader, EachColumnIsTypedIndependently) {
    TempCsv f("name,age\nann,30\nbob,25\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_TRUE(std::holds_alternative<std::string>(t.rows[0][0]));
    EXPECT_TRUE(std::holds_alternative<double>(t.rows[0][1]));
}

TEST(Loader, EmptyCellInTextColumnIsNull) {
    TempCsv f("name,age\nann,30\n,25\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_TRUE(std::holds_alternative<std::monostate>(t.rows[1][0]));
}

TEST(Loader, TrailingCommaGivesNullLastField) {
    TempCsv f("a,b\n1,\n");
    Table t = loadCsv(f.str(), "t");
    ASSERT_EQ(t.rows[0].size(), 2u);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(t.rows[0][1]));
}

TEST(Loader, AllEmptyColumnIsAllNull) {
    TempCsv f("a,b\n1,\n2,\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_TRUE(std::holds_alternative<std::monostate>(t.rows[0][1]));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(t.rows[1][1]));
}

TEST(Loader, WindowsLineEndings) {
    TempCsv f("name,age\r\na,18\r\nb,19\r\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_EQ(t.columns[1], "age");                         // no stray '\r' in header
    EXPECT_TRUE(std::holds_alternative<double>(t.rows[0][1])); // "18\r" would have failed to parse
}

TEST(Loader, BlankLinesAreSkipped) {
    TempCsv f("x\n1\n\n2\n\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_EQ(t.rows.size(), 2u);
}

TEST(Loader, NoTrailingNewline) {
    TempCsv f("x\n1\n2");
    Table t = loadCsv(f.str(), "t");
    EXPECT_EQ(t.rows.size(), 2u);
}
TEST(Loader, MissingFileThrows) {
    EXPECT_THROW(loadCsv("definitely_not_here.csv", "t"), std::runtime_error);
}

TEST(Loader, EmptyFileThrows) {
    TempCsv f("");
    EXPECT_THROW(loadCsv(f.str(), "t"), std::runtime_error);
}

TEST(Loader, TooFewFieldsThrows) {
    TempCsv f("a,b,c\n1,2\n");
    EXPECT_THROW(loadCsv(f.str(), "t"), std::runtime_error);
}

TEST(Loader, TooManyFieldsThrows) {
    TempCsv f("a,b\n1,2,3\n");
    EXPECT_THROW(loadCsv(f.str(), "t"), std::runtime_error);
}

TEST(Loader, UsersGeneralTest) {
    TempCsv f("username,password,age\na,a2,18\nb,b2,19\nc,c2,20\n");
    Table table = loadCsv(f.str(), "test");
    EXPECT_TRUE(std::holds_alternative<std::string>(table.rows[0][0]));
    EXPECT_TRUE(std::holds_alternative<double>(table.rows[0][2]));
    EXPECT_EQ(table.name, "test");
    EXPECT_EQ(table.rows.size(), 3u);
    EXPECT_EQ(table.columns.size(), 3u);
}

TEST(Loader, Utf8BomIsStrippedFromHeader) {
    TempCsv f("\xEF\xBB\xBF" "name,age\nz,5\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_EQ(t.columns[0], "name");
    EXPECT_EQ(t.colIndex.at("name"), 0u);
}

TEST(Loader, CapitalizedHeaderKeepsDisplayNameButIndexesLowercase) {
    TempCsv f("Username,AGE\na,18\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_EQ(t.columns[0], "Username");
    EXPECT_EQ(t.colIndex.at("username"), 0u);
    EXPECT_EQ(t.colIndex.at("age"), 1u);
    EXPECT_EQ(t.colIndex.count("Username"), 0u);
}

TEST(Loader, DecimalColumnIsNumeric) {
    TempCsv f("price\n9.99\n12.5\n");
    Table t = loadCsv(f.str(), "t");
    EXPECT_DOUBLE_EQ(std::get<double>(t.rows[0][0]), 9.99);
}