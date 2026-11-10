#include "parse.h"
#include <stdexcept>
using std::vector;
using std::string;

bool Parser::peekIsKeyword(const string& keyword) const {
    return peek().type == TokenType::Keyword && peek().text == keyword;
}

bool Parser::peekIsSymbol(const string& symbol) const {
    return peek().type == TokenType::Symbol && peek().text == symbol;
}

//Turns flat expressions into binary expression tree.
static std::unique_ptr<Expr> makeBinary(Expr::Kind kind, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right) {
    auto expr = std::make_unique<Expr>();
    expr->kind = kind;
    expr->left = std::move(left);
    expr->right = std::move(right);
    return expr;
}

static std::string describe(const Token& token) {
    if (token.type == TokenType::End) return "end of input";
    return "'" + token.text + "'";
}

// Returns a human-readable name for a token type.
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

std::unique_ptr<Expr> Parser::parseCondition(){
    auto left = parseAnd();
    while (peekIsKeyword("OR")) {
        advance();
        left = makeBinary(Expr::Kind::Or, std::move(left), parseAnd());
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseComparison(){
    auto node = std::make_unique<Expr>();
    node->kind = Expr::Kind::Comparison;
    node->column = expect(TokenType::Identifier, "").text;

    Token symbol = expect(TokenType::Symbol, "");
    if (symbol.text != ">" && symbol.text != "<" && symbol.text != "=" && symbol.text != "<=" && symbol.text != ">=" && symbol.text != "!=") {
        throw std::runtime_error("Expected comparison operator, got: " + describe(symbol));
    }

    node->op = symbol.text;

    Token value = advance();
    if (value.type != TokenType::Number && value.type != TokenType::String) {
        throw std::runtime_error("Expected number or string, got: " + describe(value));
    }
    node->value = value.text;
    node->valueType = value.type;
    return node;
}

std::unique_ptr<Expr> Parser::parseAnd(){
    auto left = parsePrimary();
    while (peekIsKeyword("AND")) {
        advance();
        auto right = parsePrimary();
        left = makeBinary(Expr::Kind::And, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parsePrimary(){
    if (peekIsSymbol("(")){
        advance();
        auto expr = parseCondition();
        expect(TokenType::Symbol, ")");
        return expr;
    }
    return parseComparison();
}

SelectStatement Parser::parseSelect() {
    SelectStatement stmt;
    parseSelectColumns(stmt);
    parseSelectTable(stmt);
    if (peekIsKeyword("WHERE")) {
        advance();
        stmt.where = parseCondition();
    }
    if (peek().type == TokenType::Symbol && peek().text == ";") {
        advance();
    }
    if (peek().type != TokenType::End) {
        throw std::runtime_error("Expected end of statement, got: " + describe(peek()));
    }
    return stmt;
}