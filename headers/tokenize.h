#pragma once
#include <iostream>
#include <string>
#include <vector>

enum class TokenType { Keyword, Identifier, Number, Symbol, String, End};

struct Token {
    TokenType type;
    std::string text;
};

//Tokenizes query and determines token types
std::vector<Token> parseSQLquery(std::string query);