#include "parse.h"
#include <stdexcept>
using std::vector;
using std::string;

Parser::Parser(const vector<Token>& tokens){
    this->tokens = tokens;
    this->pos = 0;
}

Token Parser::peek() const {
    if (pos >= tokens.size()) {
        return {TokenType::End, ""};
    }
    return tokens[pos];
}

Token Parser::advance() {
    Token t = peek();
    if (pos < tokens.size()) ++pos;
    return t;
}

Token Parser::expect(const Token& token, const string text) {
    if (peek().type == token.type && (text.empty() || peek().text == text)) {
        return advance();
    }
    throw std::runtime_error("Expected token: " + token.text + ", got: " + peek().text);
}

SelectStatement Parser::parseSelect() {
    SelectStatement stmt;
    expect(Token{TokenType::Keyword, "SELECT"}, "SELECT");
    if (peek().text == "*" && peek().type == TokenType::Symbol) {
        stmt.columns.push_back("*");
        advance();
    }
    else {
        stmt.columns.push_back(expect(Token{TokenType::Identifier}, "").text);
        while (peek().text == ",") {
            advance();
            stmt.columns.push_back(expect(Token{TokenType::Identifier}, "").text);
        }
    }
    expect(Token{TokenType::Keyword, "FROM"}, "FROM");
    stmt.table = expect(Token{TokenType::Identifier}, "").text;
    if (peek().text == "WHERE") {
        stmt.hasCondition = true;
        expect(Token{TokenType::Keyword, "WHERE"}, "WHERE");
        stmt.conditionColumn = expect(Token{TokenType::Identifier}, "").text;
        stmt.conditionSymbol = expect(Token{TokenType::Symbol}, "").text;
        stmt.conditionValue = expect(Token{TokenType::Number}, "").text;
    }
    if (peek().type != TokenType::End) {
        throw std::runtime_error("Expected end of statement, got: " + peek().text);
    }
    return stmt;
}