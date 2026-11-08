#include "load_file.h"
#include <gtest/gtest.h>

TEST(Loader, EmptyCellBecomesNull){
    Table table = loadCsv("data/test_with_null.csv", "test");
    EXPECT_TRUE(std::holds_alternative<double>(table.rows[0][1]));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(table.rows[1][1]));
    EXPECT_TRUE(std::holds_alternative<double>(table.rows[2][1]));
}

TEST(Loader, NumericColumnBecomesDouble){
    Table table = loadCsv("data/test_numeric.csv", "test");
    EXPECT_TRUE(std::holds_alternative<double>(table.rows[1][1]));
    EXPECT_TRUE(std::holds_alternative<double>(table.rows[2][2]));
}

TEST(Loader, UsersGeneralTest){
    Table table = loadCsv("data/users.csv", "test");
    EXPECT_TRUE(std::holds_alternative<std::string>(table.rows[0][0]));
    EXPECT_TRUE(std::holds_alternative<std::string>(table.rows[1][0]));
    EXPECT_TRUE(std::holds_alternative<std::string>(table.rows[2][0]));
    EXPECT_EQ(table.name, "test");
    EXPECT_EQ(table.rows.size(), 3);
    EXPECT_EQ(table.columns.size(), 3);
}