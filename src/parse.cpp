#include "parse.h"
#include <stdexcept>
using std::vector;
using std::string;

static std::string describe(const Token& token) {
    if (token.type == TokenType::End) return "end of input";
    return "'" + token.text + "'";
}

static std::string typeName(TokenType t){
    switch (t) {
        case TokenType::Identifier: return "identifier";
        case TokenType::Keyword: return "keyword";
        case TokenType::Number: return "number";
        case TokenType::String: return "string";
        case TokenType::Symbol: return "symbol";
        default: return "end of input";
    }
}

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

Token Parser::expect(TokenType type, const string text) {
    if (peek().type == type && (text.empty() || peek().text == text)) {
        return advance();
    }
    string wanted = text.empty() ? typeName(type) : "'" + text + "'";
    throw std::runtime_error("Expected token: " + wanted + ", got: " + describe(peek()));
}

void Parser::parseSelectColumns(SelectStatement& stmt) {
    expect(TokenType::Keyword, "SELECT");
    if (peek().text == "*" && peek().type == TokenType::Symbol) {
        stmt.columns.push_back("*");
        advance();
    }
    else {
        stmt.columns.push_back(expect(TokenType::Identifier, "").text);
        while (peek().type == TokenType::Symbol && peek().text == ",") {
            advance();
            stmt.columns.push_back(expect(TokenType::Identifier, "").text);
        }
    }
}

void Parser::parseSelectTable(SelectStatement& stmt) {
    expect(TokenType::Keyword, "FROM");
    stmt.table = expect(TokenType::Identifier, "").text;
}

void Parser::parseWhere(SelectStatement& stmt) {
    expect(TokenType::Keyword, "WHERE");
    stmt.hasCondition = true;
    stmt.conditionColumn = expect(TokenType::Identifier, "").text;

    Token symbol = expect(TokenType::Symbol, "");
    if (symbol.text != ">" && symbol.text != "<" && symbol.text != "=" && symbol.text != "<=" && symbol.text != ">=" && symbol.text != "!=") {
        throw std::runtime_error("Invalid condition symbol: " + describe(symbol));
    }
    stmt.conditionSymbol = symbol.text;
    Token value = advance();
    if (value.type != TokenType::Number && value.type != TokenType::String) {
        throw std::runtime_error("Expected number or string, got: " + describe(value));
    }
    stmt.conditionValue = value.text;
    stmt.conditionValueType = value.type;
}

SelectStatement Parser::parseSelect() {
    SelectStatement stmt;
    parseSelectColumns(stmt);
    parseSelectTable(stmt);
    if (peek().text == "WHERE" && peek().type == TokenType::Keyword) {
        parseWhere(stmt);
    }
    if (peek().type == TokenType::Symbol && peek().text == ";") {
        advance();
    }
    if (peek().type != TokenType::End) {
        throw std::runtime_error("Expected end of statement, got: " + describe(peek()));
    }
    return stmt;
}