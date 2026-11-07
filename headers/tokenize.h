#include <iostream>
#include <string>
#include <vector>
using namespace std;

enum class TokenType { Keyword, Identifier, Number, Symbol, String};

struct Token {
    TokenType type;
    string text;
};

vector<Token> parseSQLquery(string query);