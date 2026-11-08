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

    //Ensures SELECT syntax validity
    void parseSelectColumns(SelectStatement& stmt);
    //Ensures FROM syntax validity
    void parseSelectTable(SelectStatement& stmt);
    //Ensures WHERE syntax validity
    void parseWhere(SelectStatement& stmt);
    
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