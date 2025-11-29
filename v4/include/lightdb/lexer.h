#ifndef LIGHTDB_LEXER_H
#define LIGHTDB_LEXER_H

#include "sql_ast.h"
#include <string>
#include <vector>

namespace lightdb {

class Lexer {
public:
    explicit Lexer(const std::string& sql) : sql_(sql), pos_(0) {}
    std::vector<Token> Tokenize();

private:
    char CurrentChar();
    void Advance();
    void SkipWhitespace();
    Token ScanIdentifierOrKeyword();
    Token ScanNumber();
    Token ScanString();
    Token ScanSymbol();

    std::string sql_;
    size_t pos_;
};

} // namespace lightdb
#endif