#include "lightdb/lexer.h"
#include <cctype>
#include <algorithm>
#include <unordered_map>

namespace lightdb {

std::vector<Token> Lexer::Tokenize() {
    std::vector<Token> tokens;
    while (pos_ < sql_.length()) {
        SkipWhitespace();
        if (pos_ >= sql_.length()) break;

        char c = CurrentChar();
        if (isalpha(c)) {
            tokens.push_back(ScanIdentifierOrKeyword());
        } else if (isdigit(c)) {
            tokens.push_back(ScanNumber());
        } else if (c == '\'' || c == '"') {
            tokens.push_back(ScanString());
        } else {
            tokens.push_back(ScanSymbol());
        }
    }
    tokens.push_back({TokenType::END_OF_INPUT, ""});
    return tokens;
}

char Lexer::CurrentChar() {
    if (pos_ >= sql_.length()) return '\0';
    return sql_[pos_];
}

void Lexer::Advance() {
    pos_++;
}

void Lexer::SkipWhitespace() {
    while (pos_ < sql_.length() && isspace(CurrentChar())) {
        Advance();
    }
}

Token Lexer::ScanIdentifierOrKeyword() {
    std::string text;
    while (isalnum(CurrentChar()) || CurrentChar() == '_') {
        text += CurrentChar();
        Advance();
    }

    std::string upper_text = text;
    std::transform(upper_text.begin(), upper_text.end(), upper_text.begin(), ::toupper);

    static std::unordered_map<std::string, TokenType> keywords = {
        {"SELECT", TokenType::SELECT}, {"INSERT", TokenType::INSERT},
        {"DELETE", TokenType::DELETE}, {"UPDATE", TokenType::UPDATE},
        {"CREATE", TokenType::CREATE}, {"TABLE", TokenType::TABLE},
        {"FROM", TokenType::FROM}, {"WHERE", TokenType::WHERE},
        {"INTO", TokenType::INTO}, {"VALUES", TokenType::VALUES},
        {"SET", TokenType::SET}, {"AND", TokenType::AND}, {"OR", TokenType::OR},
        {"INT", TokenType::INT_TYPE}, {"VARCHAR", TokenType::VARCHAR_TYPE}
    };

    if (keywords.find(upper_text) != keywords.end()) {
        return {keywords[upper_text], upper_text};
    }
    return {TokenType::IDENTIFIER, text};
}

Token Lexer::ScanNumber() {
    std::string text;
    while (isdigit(CurrentChar())) {
        text += CurrentChar();
        Advance();
    }
    return {TokenType::INT_LITERAL, text};
}

Token Lexer::ScanString() {
    char quote = CurrentChar();
    Advance(); // Skip opening quote
    std::string text;
    while (CurrentChar() != quote && CurrentChar() != '\0') {
        text += CurrentChar();
        Advance();
    }
    if (CurrentChar() == quote) {
        Advance(); // Skip closing quote
    }
    return {TokenType::STRING_LITERAL, text};
}

Token Lexer::ScanSymbol() {
    char c = CurrentChar();
    Advance();
    switch (c) {
        case '*': return {TokenType::ASTERISK, "*"};
        case ',': return {TokenType::COMMA, ","};
        case ';': return {TokenType::SEMICOLON, ";"};
        case '(': return {TokenType::LPAREN, "("};
        case ')': return {TokenType::RPAREN, ")"};
        case '=': return {TokenType::EQ, "="};
        case '<': 
            if (CurrentChar() == '=') { Advance(); return {TokenType::LTE, "<="}; }
            return {TokenType::LT, "<"};
        case '>': 
            if (CurrentChar() == '=') { Advance(); return {TokenType::GTE, ">="}; }
            return {TokenType::GT, ">"};
        case '!':
            if (CurrentChar() == '=') { Advance(); return {TokenType::NEQ, "!="}; }
            break;
    }
    return {TokenType::IDENTIFIER, std::string(1, c)}; // Fallback
}

} // namespace lightdb