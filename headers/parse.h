#pragma once
#include <string>
#include <vector>
#include "tokenize.h"

struct SelectStatement {
    std::vector<std::string> columns;
    std::string table;
    bool hasCondition = false;
    std::string conditionColumn;
    std::string conditionSymbol;
    std::string conditionValue;
    TokenType conditionValueType = TokenType::Number;
};

class Parser {
private:
    std::vector<Token> tokens;
    size_t pos = 0;

    void parseSelectColumns(SelectStatement& stmt);
    void parseSelectTable(SelectStatement& stmt);
    void parseWhere(SelectStatement& stmt);
    
public:
    Parser(const std::vector<Token>& tokens);

    Token peek() const;

    Token advance();

    Token expect(TokenType type, const std::string text);

    SelectStatement parseSelect();
};