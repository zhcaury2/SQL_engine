#include <gtest/gtest.h>
#include "tokenize.h"
#include "parse.h"
#include <stdexcept>

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

TEST(Tokenizer, DecimalIsOneNumberToken) {
    auto t = parseSQLquery("price <= 9.99");
    ASSERT_EQ(t.size(), 3u);
    EXPECT_EQ(t[2].type, TokenType::Number);
    EXPECT_EQ(t[2].text, "9.99");
}

TEST(Tokenizer, TwoDotsInNumberThrows) {
    EXPECT_THROW(parseSQLquery("1.2.3"), std::runtime_error);
}

TEST(Tokenizer, TrailingSecondDotThrows) {
    EXPECT_THROW(parseSQLquery("x > 1.5."), std::runtime_error);
}

TEST(Tokenizer, AndOrKeywordsAreRecognized) {
    auto t = parseSQLquery("a AND b OR c");
    ASSERT_EQ(t.size(), 5u);
    EXPECT_EQ(t[1].type, TokenType::Keyword);
    EXPECT_EQ(t[1].text, "AND");
    EXPECT_EQ(t[3].type, TokenType::Keyword);
    EXPECT_EQ(t[3].text, "OR");
}

TEST(Tokenizer, AndOrAreKeywordsStoredUppercase) {
    auto t = parseSQLquery("and OR And");
    ASSERT_EQ(t.size(), 3u);
    EXPECT_EQ(t[0].type, TokenType::Keyword);
    EXPECT_EQ(t[0].text, "AND");
    EXPECT_EQ(t[1].text, "OR");
    EXPECT_EQ(t[2].text, "AND");
}

TEST(Tokenizer, WordsStartingWithOrOrAndAreIdentifiers) {
    auto t = parseSQLquery("order android origin");
    ASSERT_EQ(t.size(), 3u);
    for (const auto& tok : t) EXPECT_EQ(tok.type, TokenType::Identifier);
}

TEST(Tokenizer, ParenthesesAreSymbols) {
    auto t = parseSQLquery("(a=1)");
    ASSERT_EQ(t.size(), 5u);
    EXPECT_EQ(t[0].type, TokenType::Symbol);
    EXPECT_EQ(t[0].text, "(");
    EXPECT_EQ(t[4].text, ")");
}

TEST(Tokenizer, AndInsideStringIsNotKeyword) {
    auto t = parseSQLquery("'this AND that'");
    ASSERT_EQ(t.size(), 1u);
    EXPECT_EQ(t[0].type, TokenType::String);
    EXPECT_EQ(t[0].text, "this AND that");
}