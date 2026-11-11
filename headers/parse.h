#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "tokenize.h"

struct Expr {
    enum class Kind { Comparison, And, Or};
    Kind kind;

    std::string column;
    std::string op;
    std::string value;
    TokenType valueType = TokenType::Number;
    
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

struct OrderKey {
    std::string column;
    bool descending = false;
};

struct SelectStatement {
    std::vector<std::string> columns;
    std::string table;
    std::unique_ptr<Expr> where;
    std::vector<OrderKey> orderBy;
    std::optional<size_t> limit;
};

class Parser {
private:
    std::vector<Token> tokens;
    size_t pos = 0;

    //Ensures SELECT syntax validity
    void parseSelectColumns(SelectStatement& stmt);
    //Ensures FROM syntax validity
    void parseSelectTable(SelectStatement& stmt);
    std::unique_ptr<Expr> parseCondition();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseAnd();
    std::unique_ptr<Expr> parsePrimary();
    bool peekIsKeyword(const std::string& keyword) const;
    bool peekIsSymbol(const std::string& symbol) const;

    void parseLimit(SelectStatement& stmt);
    void parseOrderBy(SelectStatement& stmt);
    
public:
    Parser(const std::vector<Token>& tokens);

    //Peeks at the current token without advancing the parser
    Token peek() const;

    //Advances the parser to the next token
    Token advance();

    //Ensures the current token matches the expected type and text
    Token expect(TokenType type, const std::string text);

    //Parses a SELECT statement
    SelectStatement parseSelect();
};