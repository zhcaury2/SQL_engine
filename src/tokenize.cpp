#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include "../headers/tokenize.h"
#include <stdexcept>

const std::string keywords[] = {"select", "from", "where"};

bool checkKeywords(const std::string &identifier) {
    std::string lowerIdentifier = identifier;
    for (char &ch : lowerIdentifier) {
        ch = static_cast<char>(tolower(ch));
    }
    for (const std::string &keyword : keywords) {
        if (lowerIdentifier == keyword) {
            return true;
        }
    }
    return false;
}

std::vector<Token> parseSQLquery(std::string query){
    std::vector<Token> tokens; 
    size_t i = 0;
    while (i < query.size()) {
        char c = query[i];
        if (isalpha(c) || c == '_') {
            std::string identifier;
            while (i < query.size() && (isalnum(query[i]) || query[i] == '_')) {
                identifier += query[i];
                i++;
            }
            if (checkKeywords(identifier)) {
                for (char &ch : identifier) {
                    ch = static_cast<char>(toupper(ch));
                }
                tokens.push_back({TokenType::Keyword, identifier});
            } else {
                tokens.push_back({TokenType::Identifier, identifier});
            }
        } else if (isdigit(c)) {
            std::string number;
            while (i < query.size() && isdigit(query[i])) {
                number += query[i];
                i++;
            }
            tokens.push_back({TokenType::Number, number});
        } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            i++;
        } else if (c == '\'') {
            i++;
            std::string value;
            while (i < query.size() && query[i] != '\'') {
                value += query[i];
                i++;
                if (i >= query.size()) {
                    throw std::runtime_error("Unterminated string literal");
                }
            }
            i++;
            tokens.push_back({TokenType::String, value});
        } else {
            std::string symbol(1, c);
            if (c == '>' && i + 1 < query.size() && query[i + 1] == '=') {
                symbol += query[i + 1];
                i++;
            }
            else if (c == '<' && i + 1 < query.size() && query[i + 1] == '=') {
                symbol += query[i + 1];
                i++;
            }
            else if (c == '!' && i + 1 < query.size() && query[i + 1] == '=') {
                symbol += query[i + 1];
                i++;
            }
            tokens.push_back({TokenType::Symbol, symbol});
            i++;
        }
    }
    return tokens;
}