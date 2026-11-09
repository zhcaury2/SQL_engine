#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include "../headers/tokenize.h"
#include <stdexcept>
#include "helpers.h"


static const std::string keywords[] = {"select", "from", "where"};

bool checkKeywords(const std::string &identifier) {
    std::string lowerIdentifier = toLower(identifier);
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
        size_t dots = 0;
        if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string identifier;
            while (i < query.size() && (isalnum(static_cast<unsigned char>(query[i])) || query[i] == '_')) {
                identifier += query[i];
                i++;
            }
            if (checkKeywords(identifier)) {
                tokens.push_back({TokenType::Keyword, toUpper(identifier)});
            } else {
                tokens.push_back({TokenType::Identifier, toLower(identifier)});
            }
        } else if (isdigit(static_cast<unsigned char>(c))) {
            std::string number;
            while (i < query.size() && (isdigit(static_cast<unsigned char>(query[i])) || query[i] == '.')) {
                if (query[i] == '.') {
                    if (dots > 0) {
                        throw std::runtime_error("Invalid number literal");
                    }
                    dots++;
                }
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
            }
            if (i >= query.size()) {
                    throw std::runtime_error("Unterminated string literal");
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