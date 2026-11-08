#include <gtest/gtest.h>
#include "tokenize.h"
#include "parse.h"

TEST(Tokenizer, EmptyInput) {
    EXPECT_TRUE(parseSQLquery("").empty());
}

TEST(Tokenizer, CommaWithoutSpaces) {
    auto t = parseSQLquery("name,age");
    ASSERT_EQ(t.size(), 3u);
    EXPECT_EQ(t[0].text, "name");
    EXPECT_EQ(t[1].text, ",");
    EXPECT_EQ(t[2].text, "age");
}

TEST(Tokenizer, ComparisonWithoutSpaces) {
    auto t = parseSQLquery("age>21");
    ASSERT_EQ(t.size(), 3u);
    EXPECT_EQ(t[0].type, TokenType::Identifier);
    EXPECT_EQ(t[1].type, TokenType::Symbol);
    EXPECT_EQ(t[2].type, TokenType::Number);
    EXPECT_EQ(t[2].text, "21");
}

TEST(Tokenizer, ExtraWhitespaceIsIgnored) {
    auto t = parseSQLquery("   name    age  ");
    ASSERT_EQ(t.size(), 2u);
}

TEST(Tokenizer, KeywordsAreRecognizedAnyCase) {
    auto t = parseSQLquery("select name from users");
    ASSERT_EQ(t.size(), 4u);
    EXPECT_EQ(t[0].type, TokenType::Keyword);
    EXPECT_EQ(t[0].text, "SELECT");
    EXPECT_EQ(t[2].type, TokenType::Keyword);
}

TEST(Tokenizer, QuotedString) {
    auto t = parseSQLquery("name='Alice'");
    ASSERT_EQ(t.size(), 3u);
    EXPECT_EQ(t[0].text, "name");
    EXPECT_EQ(t[1].text, "=");
    EXPECT_EQ(t[2].text, "Alice");
}

TEST(Tokenizer, MultiCharacterSymbols) {
    auto t = parseSQLquery("age>=21");
    ASSERT_EQ(t.size(), 3u);
    EXPECT_EQ(t[1].text, ">=");
}

TEST(Tokenizer, LessThanOrEqual) {
    auto t = parseSQLquery("age<=21");
    ASSERT_EQ(t.size(), 3u);
    EXPECT_EQ(t[1].text, "<=");
}

TEST(Tokenizer, NotEqual) {
    auto t = parseSQLquery("age!=21");
    ASSERT_EQ(t.size(), 3u);
    EXPECT_EQ(t[1].text, "!=");
}