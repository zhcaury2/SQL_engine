#pragma once
#include <string>
#include <vector>
#include "tokenize.h"

struct SelectStatement {
    std::vector<std::string> columns;
    std::string table;
    bool hasCondition;
    std::string conditionColumn;
    std::string conditionSymbol;
    std::string conditionValue;
};

class Parser {
private:
    std::vector<Token> tokens;
    size_t pos = 0;
public:
    Parser(const std::vector<Token>& tokens);

    Token peek() const;

    Token advance();

    Token expect(const Token& token, const std::string text);

    SelectStatement parseSelect();
};